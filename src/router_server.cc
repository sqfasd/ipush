#include "deps/jsoncpp/include/json/json.h"
#include "deps/base/flags.h"
#include "deps/base/logging.h"
#include "router_server.h"
#include "deps/base/time.h"
#include "deps/base/string_util.h"
#include "deps/limonp/StringUtil.hpp"

DEFINE_string(sserver_sub_ips, "127.0.0.1|127.0.0.1", "");
DEFINE_string(sserver_sub_ports, "8100|8200", "");
DEFINE_string(sserver_sub_uri, "/stream?cname=12", "");

DEFINE_string(sserver_pub_ips, "127.0.0.1|127.0.0.1", "");
DEFINE_string(sserver_pub_ports, "8100|8200", "");

DEFINE_string(ssdb_path, "/tmp/ssdb_tmp", "");

//DEFINE_string(sserver_pub_uri, "/pub?cname=12&content=123", "");
DEFINE_int32(retry_interval, 2, "seconds");
DEFINE_bool(libevent_debug_log, false, "for debug logging");

const struct timeval RETRY_TV = {FLAGS_retry_interval, 0};

namespace xcomet {

using Limonp::string_format;

RouterServer::RouterServer() { 
  evbase_ = event_base_new();
  CHECK(evbase_);
}

RouterServer::~RouterServer() {
  event_base_free(evbase_);
}

void RouterServer::Start() {
  InitSubCliAddrs(FLAGS_sserver_sub_ips, FLAGS_sserver_sub_ports);
  InitSubClients();
  InitPubCliAddrs(FLAGS_sserver_pub_ips, FLAGS_sserver_pub_ports);
  InitPubClients();
  InitStorage(FLAGS_ssdb_path);
  event_base_dispatch(evbase_);
}

void RouterServer::ResetSubClient(size_t id) {
  VLOG(5) << "ResetSubClient";
  CHECK(session_sub_clients_.size());
  CHECK(subcliaddrs_.size());
  CHECK(id < subcliaddrs_.size()) ;
  session_sub_clients_[id].reset(
                new SessionSubClient(
                    this,
                    evbase_, 
                    id,
                    //ClientErrorCB,
                    subcliaddrs_[id].first, 
                    subcliaddrs_[id].second,
                    FLAGS_sserver_sub_uri
                    )
              );
  session_sub_clients_[id]->MakeSubEvent();
}

void RouterServer::InitSubCliAddrs(const string& ipsstr, const string& portsstr) {
  VLOG(5) << FLAGS_sserver_sub_ips ;
  VLOG(5) << FLAGS_sserver_sub_ports ;
  vector<string> ips;
  vector<string> ports;
  SplitString(ipsstr, '|', &ips);
  SplitString(portsstr, '|', &ports);
  CHECK(ips.size());
  CHECK(ips.size() == ports.size());
  subcliaddrs_.resize(ips.size());
  LOG(INFO) << "subcliaddrs_ size : " << subcliaddrs_.size();
  for(size_t i = 0; i < subcliaddrs_.size(); i++) {
    subcliaddrs_[i].first = ips[i];
    subcliaddrs_[i].second = atoi(ports[i].c_str());
  }
}

void RouterServer::InitPubCliAddrs(const string& ipsstr, const string& portsstr) {
  VLOG(5) << FLAGS_sserver_pub_ips ;
  VLOG(5) << FLAGS_sserver_pub_ports ;
  vector<string> ips;
  vector<string> ports;
  SplitString(ipsstr, '|', &ips);
  SplitString(portsstr, '|', &ports);
  CHECK(ips.size());
  CHECK(ips.size() == ports.size());
  pubcliaddrs_.resize(ips.size());
  LOG(INFO) << "pubcliaddrs_ size : " << pubcliaddrs_.size();
  for(size_t i = 0; i < pubcliaddrs_.size(); i++) {
    pubcliaddrs_[i].first = ips[i];
    pubcliaddrs_[i].second = atoi(ports[i].c_str());
  }
}

void RouterServer::InitSubClients() {
  CHECK(session_sub_clients_.empty());
  for (size_t i = 0; i < subcliaddrs_.size(); i++) {
    VLOG(5) << "new SessionSubClient " << subcliaddrs_[i].first << "," << subcliaddrs_[i].second;
    session_sub_clients_.push_back(
                shared_ptr<SessionSubClient>(
                    new SessionSubClient(
                        this,
                        evbase_, 
                        i,
                        //ClientErrorCB,
                        subcliaddrs_[i].first, 
                        subcliaddrs_[i].second,
                        FLAGS_sserver_sub_uri
                        )
                    )
                );
  }
  // init request events
  for(size_t i = 0; i < session_sub_clients_.size(); i++) {
    session_sub_clients_[i]->MakeSubEvent();
  }
}

void RouterServer::InitPubClients() {
  CHECK(session_pub_clients_.empty());
  for (size_t i = 0; i < pubcliaddrs_.size(); i++) {
    VLOG(5) << "new SessionPubClient " << pubcliaddrs_[i].first << "," << pubcliaddrs_[i].second;
    session_pub_clients_.push_back(
                shared_ptr<SessionPubClient>(
                    new SessionPubClient(
                        this,
                        evbase_, 
                        i,
                        //ClientErrorCB,
                        pubcliaddrs_[i].first, 
                        pubcliaddrs_[i].second
                        )
                    )
                );
  }
}

void RouterServer::InitStorage(const string& path) {
  LOG(INFO) << "InitStorage path:" << path;
  storage_.reset(new Storage(evbase_, path));
  
}

void RouterServer::ClientErrorCB(int sock, short which, void *arg) {
  VLOG(5) << "ClientErrorCB";
  CliErrInfo * err = static_cast<CliErrInfo*>(arg);
  CHECK(err != NULL);
  err->router->ResetSubClient(err->id);
  delete err;
}

void RouterServer::MakeCliErrEvent(CliErrInfo* clierr) {
  VLOG(5) << "MakeCliErrEvent";
  struct event * everr = event_new(evbase_, -1, EV_READ|EV_TIMEOUT, ClientErrorCB, static_cast<void *>(clierr));
  event_add(everr, &RETRY_TV);
  LOG(INFO) << "event_add error_event : timeout " << FLAGS_retry_interval;
} 

void RouterServer::MakePubEvent(size_t clientid, const char* pub_uri) {
  VLOG(5) << "RouterServer::MakePubEvent";
  if(clientid >= session_pub_clients_.size()) {
    LOG(ERROR) << "clientid " << clientid << " out of range";
    return;
  }
  session_pub_clients_[clientid]->MakePubEvent(pub_uri);
}

void RouterServer::ChunkedMsgHandler(size_t clientid, const char* buffer, size_t len) {
  VLOG(5) << "RouterServer::ChunkedMsgHandler";
  Json::Value value;
  Json::Reader reader;
  if(!reader.parse(buffer, buffer + len, value)) {
    LOG(ERROR) << "json parse failed, data" << string(buffer, len);
    return;
  }
  const Json::Value& type = value["type"];
  if(type.asString() == "user_msg") {
    VLOG(5) << type;
    string uri = string_format("/pub?cname=12&content=%s", type.asCString());
    const char * to_uid = value["to_uid"].asCString();
    CHECK(to_uid != NULL);
    int clientid = FindServerIdByUid(to_uid);
    if(clientid != INVALID_SID) {
      MakePubEvent(clientid, uri.c_str());
    } else {
      //TODO offline message handle
      VLOG(5) << "offline mesage";
    }
  } else if(type.asString() == "login") {
    VLOG(5) << type;
    const char * uid = value["uid"].asCString();
    CHECK(uid != NULL);
    InsertUid(uid, clientid);
    LOG(INFO) << "uid: " << uid << " clientid: " << clientid;
  } else {
    LOG(ERROR) << value.asString();
  }
}

void RouterServer::InsertUid(const UserID& uid, SessionServerID sid) {
  u2sMap_[uid] = sid;
  VLOG(5) << "u2sMap size: " << u2sMap_.size() ;
}

SessionServerID RouterServer::FindServerIdByUid(const UserID& uid) const {
  SessionServerID sid;
  map<UserID, SessionServerID>::const_iterator citer;
  citer = u2sMap_.find(uid);
  if(citer == u2sMap_.end()) {
    return INVALID_SID;
  }
  return citer->second;
}

} // namespace xcomet

int main(int argc, char ** argv) {
  base::ParseCommandLineFlags(&argc, &argv, false);
  if(FLAGS_libevent_debug_log) {
    event_enable_debug_logging(EVENT_DBG_ALL);
  }
  xcomet::RouterServer server;
  server.Start();
  return EXIT_SUCCESS;
}

