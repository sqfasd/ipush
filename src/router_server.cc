#include "deps/jsoncpp/include/json/json.h"
#include "deps/base/flags.h"
#include "deps/base/logging.h"
#include "router_server.h"
#include "deps/base/time.h"
#include "deps/base/string_util.h"
#include "deps/limonp/StringUtil.hpp"
#include "http_query.h"
#include <event2/listener.h>

DEFINE_string(admin_listen_ip, "0.0.0.0", "");
DEFINE_int32(admin_listen_port, 9100, "");

DEFINE_string(sserver_sub_ips, "127.0.0.1|127.0.0.1", "");
DEFINE_string(sserver_sub_ports, "8100|8200", "");
DEFINE_string(sserver_sub_uri, "/stream?cname=12", "");

DEFINE_string(sserver_pub_ips, "127.0.0.1|127.0.0.1", "");
DEFINE_string(sserver_pub_ports, "8100|8200", "");

DEFINE_string(ssdb_path, "/tmp/ssdb_tmp", "");

//DEFINE_string(sserver_pub_uri, "/pub?cname=12&content=123", "");
DEFINE_int32(retry_interval, 2, "seconds");
DEFINE_bool(libevent_debug_log, false, "for debug logging");

DEFINE_string(admin_uri_presence, "/presence", "");
DEFINE_string(admin_uri_broadcast, "/broadcast", "");
DEFINE_string(admin_uri_pub, "/pub", "");
DEFINE_string(admin_uri_offmsg, "/offmsg", "");


#define IS_LOGIN(type) (strcmp(type, "login") == 0)
#define IS_LOGOUT(type) (strcmp(type, "logout") == 0)
#define IS_MSG(type) (strcmp(type, "user_msg") == 0)
#define IS_NOOP(type) (strcmp(type, "noop") == 0)

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
  InitAdminHttp();
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

void RouterServer::InitAdminHttp() {
  admin_http_ = evhttp_new(evbase_);
  evhttp_set_cb(admin_http_, FLAGS_admin_uri_pub.c_str(), AdminPubCB, this);
  evhttp_set_cb(admin_http_, FLAGS_admin_uri_broadcast.c_str(), AdminBroadcastCB, this);
  evhttp_set_cb(admin_http_, FLAGS_admin_uri_presence.c_str(), AdminCheckPresenceCB, this);
  evhttp_set_cb(admin_http_, FLAGS_admin_uri_offmsg.c_str(), AdminCheckOffMsgCB, this);
  
  struct evhttp_bound_socket* sock;
  sock = evhttp_bind_socket_with_handle(admin_http_, FLAGS_admin_listen_ip.c_str(), FLAGS_admin_listen_port);
  CHECK(sock) << "bind address failed" << strerror(errno);
  LOG(INFO) << "admin server listen on" << FLAGS_admin_listen_ip << " " << FLAGS_admin_listen_port;
  struct evconnlistener * listener = evhttp_bound_socket_get_listener(sock);
  evconnlistener_set_error_cb(listener, AcceptErrorCB);
}

void RouterServer::AcceptErrorCB(struct evconnlistener * listener, void * ctx) {
  LOG(ERROR) << "RouterServer::AcceptErrorCB";
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

void RouterServer::MakePubEvent(const char* uid, const char* data, size_t len) {
  VLOG(5) << "RouterServer::MakePubEvent";
  SessionServerID sid = FindServerIdByUid(uid);
  if(sid == INVALID_SID) {
    LOG(INFO) << "uid " << uid << " is offline";
    //to save the pub the content TODO
  } else {
    CHECK(size_t(sid) < session_pub_clients_.size());
    session_pub_clients_[sid]->MakePubEvent(uid, data, len);
    VLOG(5) << "session_pub_clients_[sid]->MakePubEvent";
  }
}

void RouterServer::ChunkedMsgHandler(size_t clientid, const char* buffer, size_t len) {
  VLOG(5) << "RouterServer::ChunkedMsgHandler";
  Json::Value value;
  Json::Reader reader;
  if(!reader.parse(buffer, buffer + len, value)) {
    LOG(ERROR) << "json parse failed, data" << string(buffer, len);
    return;
  }
  const char* type = NULL;
  try {
    type = value["type"].asCString();
  } catch (...) {
    LOG(ERROR) << "json catch exception :" << string(buffer, len);
    return;
  }
  if (type == NULL) {
    LOG(ERROR) << "type is NULL";
    return;
  }
  if(IS_MSG(type)) {
    VLOG(5) << type;
    string uri = string_format("/pub?cname=12&content=%s", type);
    const char * uid = value["uid"].asCString();
    CHECK(uid != NULL);
    MakePubEvent(uid, buffer, len);
  } else if(IS_LOGIN(type)) {
    VLOG(5) << type;
    const char * uid = value["uid"].asCString();
    CHECK(uid != NULL);
    InsertUid(uid, clientid);
    LOG(INFO) << "uid: " << uid << " clientid: " << clientid;
    // pop offline message
    storage_->PopOfflineMessageIterator(uid, boost::bind(&RouterServer::PopOfflineMsgDoneCB, this, uid, _1));
  } else if(IS_LOGOUT(type)) {
    VLOG(5) << type;
    //TODO
    //LOG(ERROR) << value;
    //LOG(ERROR) << value.asString(); // exception
  } else if(IS_NOOP(type)) {
    VLOG(5) << type;
  } else {
    LOG(ERROR) << "type is illegal";
  }
}

void RouterServer::InsertUid(const UserID& uid, SessionServerID sid) {
  u2sMap_[uid] = sid;
  VLOG(5) << "u2sMap size: " << u2sMap_.size() ;
}

SessionServerID RouterServer::FindServerIdByUid(const UserID& uid) const {
  map<UserID, SessionServerID>::const_iterator citer;
  citer = u2sMap_.find(uid);
  if(citer == u2sMap_.end()) {
    return INVALID_SID;
  }
  return citer->second;
}

void RouterServer::PushOfflineMsgDoneCB(bool ok) {
  VLOG(5) << "SaveOfflineMsgCB " << ok;
}

void RouterServer::AdminPubCB(struct evhttp_request* req, void *ctx) {
  RouterServer * self = static_cast<RouterServer*>(ctx);
  VLOG(5) << "RouterServer::AdminPubCB";

  HttpQuery query(req);
  const char * uid = query.GetStr("uid", NULL);
  //const char * content = query.GetStr("content", NULL);
  if (uid == NULL) {
    LOG(ERROR) << "uid not found";
    self->ReplyError(req);
    return;
  }
  
  struct evbuffer* input_buffer = evhttp_request_get_input_buffer(req);
  int len = evbuffer_get_length(input_buffer);
  VLOG(5) << "data length:" << len;
  const char * bufferstr = (const char*)evbuffer_pullup(input_buffer, len);
  if(bufferstr == NULL) {
    LOG(ERROR) << "evbuffer_pullup return null";
    self->ReplyError(req);
    return;
  }
  self->MakePubEvent(uid, bufferstr, len);
  self->ReplyOK(req);
  VLOG(5) << "reply ok";
}

void RouterServer::AdminBroadcastCB(struct evhttp_request* req, void *ctx) {
  //TODO
  //RouterServer * self = static_cast<RouterServer*>(ctx);
  VLOG(5) << "RouterServer::AdminBroadcastCB";
}

void RouterServer::AdminCheckPresenceCB(struct evhttp_request* req, void *ctx) {
  RouterServer * self = static_cast<RouterServer*>(ctx);
  VLOG(5) << "RouterServer::AdminCheckPresenceCB";
  string response;
  string_format(response,  "{\"presence\": %d}", self->u2sMap_.size());
  SendReply(req, response.c_str(), response.size());
  VLOG(5) << response;
}

void RouterServer::AdminCheckOffMsgCB(struct evhttp_request* req, void *ctx) {
  RouterServer * self = static_cast<RouterServer*>(ctx);
  
  HttpQuery query(req);
  const char * uid = query.GetStr("uid", NULL);
  if (uid == NULL) {
    LOG(ERROR) << "uid not found";
    self->ReplyError(req);
    return;
  }
  self->storage_->GetOfflineMessageIterator(uid, boost::bind(&RouterServer::GetOfflineMsgDoneCB, self, UserID(uid), req, _1));
  //TODO
}

void RouterServer::SendReply(struct evhttp_request* req, const char* content, size_t len) {
  evhttp_add_header(req->output_headers, "Content-Type", "text/json; charset=utf-8");
  struct evbuffer * output_buffer = evhttp_request_get_output_buffer(req);
  evbuffer_add(output_buffer, content, len);
  evhttp_send_reply(req, HTTP_OK, "OK", output_buffer);
}

void RouterServer::PopOfflineMsgDoneCB(UserID uid, MessageIteratorPtr mit) {
  VLOG(5) << "PopOfflineMsgDoneCB";
  SessionServerID sid = FindServerIdByUid(uid);
  if(sid == INVALID_SID) {
    // push back to offline message queue
    return;
  }
  while(mit->HasNext()) {
    string msg = mit->Next();
    VLOG(5) << msg;
    //TODO
    //session_pub_clients_[sid]->MakePubEvent();
  }
}

void RouterServer::GetOfflineMsgDoneCB(UserID uid, struct evhttp_request * req, MessageIteratorPtr mit) {
  VLOG(5) << "RouterServer::GetOfflineMsgDoneCB";
  string msg;
  string msgs;
  while(mit->HasNext()) {
    msg = mit->Next();
    msgs += msg;
    VLOG(5) << msg;
  }
  string resp;
  string_format(resp, "{uid: \"%s\", content: \"%s\"}", uid.c_str(), msgs.c_str());
  SendReply(req, resp.c_str(), resp.size());
}

void RouterServer::ReplyError(struct evhttp_request* req) {
  VLOG(5) << "ReplyError";
  evhttp_add_header(req->output_headers, "Content-Type", "text/json; charset=utf-8");
  struct evbuffer * output_buffer = evhttp_request_get_output_buffer(req);
  const char * response = "{\"type\":\"error\"}\n"; //TODO
  evbuffer_add(output_buffer, response, strlen(response)); // TODO
  evhttp_send_reply(req, HTTP_BADREQUEST, "Error", output_buffer);
}

void RouterServer::ReplyOK(struct evhttp_request* req) {
  evhttp_add_header(req->output_headers, "Content-Type", "text/json; charset=utf-8");
  struct evbuffer * output_buffer = evhttp_request_get_output_buffer(req);
  const char * response = "{\"type\":\"ok\"}\n"; //TODO
  evbuffer_add(output_buffer, response, strlen(response)); // TODO
  evhttp_send_reply(req, HTTP_OK, "OK", output_buffer);
}

} // namespace xcomet

int main(int argc, char ** argv) {
  base::ParseCommandLineFlags(&argc, &argv, false);
  if(FLAGS_libevent_debug_log) {
    //event_enable_debug_logging(EVENT_DBG_ALL);
  }
  xcomet::RouterServer server;
  server.Start();
  return EXIT_SUCCESS;
}

