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

DEFINE_string(sserver_sub_addrs, "127.0.0.1:8100,127.0.0.1:8200", "");

DEFINE_string(sserver_sub_uri, "/rsub", "");

DEFINE_string(sserver_pub_addrs, "127.0.0.1:8100,127.0.0.1:8200", "");

DEFINE_string(ssdb_path, "/tmp/ssdb_tmp", "");

DEFINE_bool(libevent_debug_log, false, "for debug logging");

DEFINE_string(admin_uri_presence, "/presence", "");
DEFINE_string(admin_uri_broadcast, "/broadcast", "");
DEFINE_string(admin_uri_pub, "/pub", "");
DEFINE_string(admin_uri_offmsg, "/offmsg", "");

DEFINE_int64(message_batch_size, 100, "get message limit 100");

#define IS_LOGIN(type) (strcmp(type, "login") == 0)
#define IS_LOGOUT(type) (strcmp(type, "logout") == 0)
#define IS_MSG(type) (strcmp(type, "user_msg") == 0)
#define IS_NOOP(type) (strcmp(type, "noop") == 0)

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
  InitSubClients();
  InitStorage();
  InitAdminHttp();
  event_base_dispatch(evbase_);
}

void RouterServer::InitSubClients() {
  vector<string> addrs;
  SplitString(FLAGS_sserver_sub_addrs, ',', &addrs);
  CHECK(addrs.size());
  CHECK(sub_clients_.empty());
  for (size_t i = 0; i < addrs.size(); i++) {
    HttpClientOption option;
    option.id = (int)i;
    ParseIpPort(addrs[i], option.host, option.port);
    option.method = EVHTTP_REQ_GET;
    option.path = FLAGS_sserver_sub_uri;

    VLOG(5) << "new HttpClient " << option;
    shared_ptr<HttpClient> client(new HttpClient(evbase_, option, this));
    client->SetRequestDoneCallback(OnSubRequestDone);
    client->SetChunkCallback(OnSubMsg);

    sub_clients_.push_back(client);
  }
  for(size_t i = 0; i < sub_clients_.size(); i++) {
    sub_clients_[i]->StartRequest();
  }
}

void RouterServer::InitStorage() {
  LOG(INFO) << "InitStorage path:" << FLAGS_ssdb_path;
  storage_.reset(new Storage(evbase_, FLAGS_ssdb_path));
}

void RouterServer::InitAdminHttp() {
  admin_http_ = evhttp_new(evbase_);
  evhttp_set_cb(admin_http_, FLAGS_admin_uri_pub.c_str(), OnAdminPub, this);
  evhttp_set_cb(admin_http_, FLAGS_admin_uri_broadcast.c_str(), OnAdminBroadcast, this);
  evhttp_set_cb(admin_http_, FLAGS_admin_uri_presence.c_str(), OnAdminCheckPresence, this);
  evhttp_set_cb(admin_http_, FLAGS_admin_uri_offmsg.c_str(), OnAdminCheckOffMsg, this);
  
  struct evhttp_bound_socket* sock;
  sock = evhttp_bind_socket_with_handle(admin_http_, FLAGS_admin_listen_ip.c_str(), FLAGS_admin_listen_port);
  CHECK(sock) << "bind address failed" << strerror(errno);
  LOG(INFO) << "admin server listen on" << FLAGS_admin_listen_ip << " " << FLAGS_admin_listen_port;
  struct evconnlistener * listener = evhttp_bound_socket_get_listener(sock);
  evconnlistener_set_error_cb(listener, OnAcceptError);
}

void RouterServer::OnAcceptError(struct evconnlistener * listener, void * ctx) {
  LOG(ERROR) << "RouterServer::OnAcceptError";
}

//void RouterServer::MakePubEvent(const char* uid, const char* data, size_t len) {
//  VLOG(5) << "RouterServer::MakePubEvent";
//  Sid sid = FindSidByUid(uid);
//  if(sid == INVALID_SID) {
//    LOG(INFO) << "uid " << uid << " is offline";
//  } else {
//    CHECK(size_t(sid) < session_pub_clients_.size());
//    //session_pub_clients_[sid]->MakePubEvent(uid, data, len);
//    VLOG(5) << "session_pub_clients_[sid]->MakePubEvent";
//  }
//}

void RouterServer::OnSubMsg(const HttpClient* client, const string& msg, void *ctx) {
  VLOG(5) << "RouterServer::OnSubMsg";
  RouterServer* self  = (RouterServer*)ctx;
  Json::Value value;
  Json::Reader reader;
  if(!reader.parse(msg, value)) {
    LOG(ERROR) << "json parse failed, msg:" << msg;
    return;
  }
  const char* type = NULL;
  try {
    type = value["type"].asCString();
  } catch (...) {
    LOG(ERROR) << "json catch exception :" << msg;
    return;
  }
  if (type == NULL) {
    LOG(ERROR) << "type is NULL";
    return;
  }
  if(IS_MSG(type)) {
    VLOG(5) << type;
  } else if(IS_LOGIN(type)) {
    VLOG(5) << type;
    string uid;
    int64_t seq;
    try {
      uid = value["uid"].asString();
      seq = value["seq"].asInt64(); //TODO testing
    } catch (...) {
      LOG(ERROR) << "uid or seq not found";
      return;
    }
    if(uid.empty()) {
      LOG(ERROR) << "uid not found";
      return;
    }
    Sid sid = client->GetOption().id;
    self->InsertUid(uid, sid);
    VLOG(5) << "uid: " << uid << " sid: " << sid << " seq:" << seq;
    // if seq = -1, not pub message.
    if(seq >=0) {
      self->GetMsg(uid, seq, boost::bind(&RouterServer::OnGetMsg, self, uid, seq, _1));
    } else {
      VLOG(4) << "seq :" << seq; 
    }
  } else if(IS_LOGOUT(type)) {
    VLOG(5) << type;
    const char* uid;
    try {
      uid = value["uid"].asCString();
    } catch (...) {
      LOG(ERROR) << "uid not found";
      return;
    }
    if(uid == NULL) {
      LOG(ERROR) << "uid not found";
      return;
    }
    self->EraseUid(uid);
  } else if(IS_NOOP(type)) {
    VLOG(5) << type;
  } else {
    LOG(ERROR) << "type is illegal";
  }
}

void RouterServer::OnSubRequestDone(const HttpClient* client, const string& resp, void *ctx) {
  VLOG(5) << "RouterServer::OnSubRequestDone";
  RouterServer* self = (RouterServer*)ctx;
  CHECK(self);
}

void RouterServer::GetMsg(const UserID& uid, int64_t start, 
            boost::function<void (MessageIteratorPtr)> cb) {
  int64_t end = start + FLAGS_message_batch_size - 1;
  VLOG(5) << "GetMsg uid " << uid << " start:" << start << " end:" << end;
  storage_->GetMessageIterator(uid, start, end, cb);
}

void RouterServer::InsertUid(const UserID& uid, Sid sid) {
  VLOG(5) << "RouterServer::InsertUid " << uid;
  u2sMap_[uid] = sid;
  VLOG(5) << "u2sMap size: " << u2sMap_.size();
}

void RouterServer::EraseUid(const UserID& uid) {
  VLOG(5) << "RouterServer::EraseUid " << uid;
  map<UserID, Sid>::iterator iter;
  iter = u2sMap_.find(uid);
  if(iter == u2sMap_.end()) {
    LOG(ERROR) << "uid " << uid << " not found.";
    return;
  }
  u2sMap_.erase(iter);
  VLOG(3) << "erase uid : " << uid;
  VLOG(5) << "u2sMap size: " << u2sMap_.size();
}

Sid RouterServer::FindSidByUid(const UserID& uid) const {
  map<UserID, Sid>::const_iterator citer;
  citer = u2sMap_.find(uid);
  if(citer == u2sMap_.end()) {
    return INVALID_SID;
  }
  return citer->second;
}

void RouterServer::OnPushMsgDone(bool ok) {
  VLOG(5) << "OnPushMsgDone " << ok;
  if(!ok) {
    LOG(ERROR) << "push failed";
    return;
  }
}

void RouterServer::OnAdminPub(struct evhttp_request* req, void *ctx) {
  RouterServer * self = static_cast<RouterServer*>(ctx);
  VLOG(5) << "RouterServer::OnAdminPub";

  HttpQuery query(req);
  const char * uid = query.GetStr("uid", NULL);
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
  self->storage_->SaveMessage(uid, string(bufferstr, len), boost::bind(&RouterServer::OnPushMsgDone, self, _1));
  VLOG(5) << "storage_->SaveMessage" << uid << string(bufferstr, len);
  //self->MakePubEvent(uid, bufferstr, len);
  //self->ReplyOK(req);
  VLOG(5) << "reply ok";
}

void RouterServer::OnAdminBroadcast(struct evhttp_request* req, void *ctx) {
  VLOG(5) << "RouterServer::OnAdminBroadcast";
  
}

void RouterServer::OnAdminCheckPresence(struct evhttp_request* req, void *ctx) {
  RouterServer * self = static_cast<RouterServer*>(ctx);
  VLOG(5) << "RouterServer::OnAdminCheckPresence";
  string response;
  string_format(response,  "{\"presence\": %d}", self->u2sMap_.size());
  SendReply(req, response.c_str(), response.size());
  VLOG(5) << response;
}

void RouterServer::OnAdminCheckOffMsg(struct evhttp_request* req, void *ctx) {
  RouterServer * self = static_cast<RouterServer*>(ctx);
  
  HttpQuery query(req);
  const char * uid = query.GetStr("uid", NULL);
  if (uid == NULL) {
    LOG(ERROR) << "uid not found";
    self->ReplyError(req);
    return;
  }
  self->storage_->GetMessageIterator(uid, boost::bind(&RouterServer::OnGetMsgToReply, self, UserID(uid), req, _1));
}

void RouterServer::SendReply(struct evhttp_request* req, const char* content, size_t len) {
  evhttp_add_header(req->output_headers, "Content-Type", "text/json; charset=utf-8");
  struct evbuffer * output_buffer = evhttp_request_get_output_buffer(req);
  evbuffer_add(output_buffer, content, len);
  evhttp_send_reply(req, HTTP_OK, "OK", output_buffer);
}

void RouterServer::OnGetMsg(
            UserID uid, int64_t start, MessageIteratorPtr mit
            ) {
  VLOG(5) << "OnGetMsg, uid:" << uid << " start: "<< start;
  string msg;
  size_t i = 0;
  while(mit->HasNext()) {
    msg = mit->Next();
    VLOG(5) << msg;
    //MakePubEvent(uid.c_str(), msg.c_str(), msg.size());
    i++;
  }
  // message is unfinished.
  if(i == size_t(FLAGS_message_batch_size)) {
    int64_t nextstart = start + FLAGS_message_batch_size;
    GetMsg(uid, nextstart, boost::bind(&RouterServer::OnGetMsg, this, uid, nextstart, _1));
  }
}

void RouterServer::OnGetMsgToReply(UserID uid, struct evhttp_request * req, MessageIteratorPtr mit) {
  VLOG(5) << "RouterServer::OnGetMsgToReply";
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
  const char * response = "{\"type\":\"error\"}\n";
  evbuffer_add(output_buffer, response, strlen(response)); 
  evhttp_send_reply(req, HTTP_BADREQUEST, "Error", output_buffer);
}

void RouterServer::ReplyOK(struct evhttp_request* req) {
  evhttp_add_header(req->output_headers, "Content-Type", "text/json; charset=utf-8");
  struct evbuffer * output_buffer = evhttp_request_get_output_buffer(req);
  const char * response = "{\"type\":\"ok\"}\n"; 
  evbuffer_add(output_buffer, response, strlen(response)); 
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

