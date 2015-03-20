#include "src/router_server.h"

#include <event2/listener.h>
#include "deps/jsoncpp/include/json/json.h"
#include "deps/base/flags.h"
#include "deps/base/logging.h"
#include "deps/base/time.h"
#include "deps/base/string_util.h"
#include "deps/limonp/StringUtil.hpp"
#include "src/http_query.h"

DEFINE_string(admin_listen_ip, "0.0.0.0", "");
DEFINE_int32(admin_listen_port, 8100, "");

DEFINE_string(session_server_addrs, "127.0.0.1:9100", "");
DEFINE_string(session_server_sub_uri, "/rsub", "");
DEFINE_string(ssdb_path, "/tmp/ssdb_tmp", "");
DEFINE_bool(libevent_debug_log, false, "for debug logging");
DEFINE_int64(message_batch_size, 100, "get message limit 100");

#define IS_LOGIN(type) (type == "login")
#define IS_LOGOUT(type) (type == "logout")
#define IS_NOOP(type) (type == "noop")
#define IS_SUB(type) (type == "sub")
#define IS_UNSUB(type) (type == "unsub")
#define IS_MESSAGE(type) (type == "msg")
#define IS_ACK(type) (type == "ack")

namespace xcomet {

using Limonp::string_format;

const Sid INVALID_SID = -1;

RouterServer::RouterServer() {
  evbase_ = event_base_new();
  CHECK(evbase_);
}

RouterServer::~RouterServer() {
  event_base_free(evbase_);
}

void RouterServer::Start() {
  OpenSessionClients();
  InitStorage();
  InitAdminHttp();
  event_base_dispatch(evbase_);
}

void RouterServer::OpenSessionClients() {
  vector<string> addrs;
  SplitString(FLAGS_session_server_addrs, ',', &addrs);
  CHECK(addrs.size());
  CHECK(session_clients_.empty());
  session_clients_.resize(addrs.size());
  for (size_t i = 0; i < session_clients_.size(); i++) {
    HttpClientOption option;
    option.id = (int)i;
    ParseIpPort(addrs[i], option.host, option.port);
    option.method = EVHTTP_REQ_GET;
    option.path = FLAGS_session_server_sub_uri;

    VLOG(5) << "new HttpClient: " << option;
    session_clients_[i].reset(new HttpClient(evbase_, option, this));
    OpenSessionClient(i);
  }
}

void RouterServer::OpenSessionClient(Sid sid) {
  VLOG(4) << "RouterServer::OpenSessionClient: " << sid;
  CHECK(sid >= 0 && size_t(sid) < session_clients_.size());
  session_clients_[sid]->Init();
  session_clients_[sid]->SetRequestDoneCallback(OnSessionRequestDone);
  session_clients_[sid]->SetChunkCallback(OnSessionMsg);
  session_clients_[sid]->StartRequest();
}

void RouterServer::CloseSessionClient(Sid sid) {
  VLOG(4) << "RouterServer::CloseSessionClient: " << sid;
  CHECK(sid >= 0 && size_t(sid) < session_clients_.size());
  session_clients_[sid]->Free();
}

void RouterServer::InitStorage() {
  LOG(INFO) << "InitStorage path:" << FLAGS_ssdb_path;
  RemoteStorageOption option;
  storage_.reset(new RemoteStorage(evbase_));
}

void RouterServer::InitAdminHttp() {
  admin_http_ = evhttp_new(evbase_);
  evhttp_set_cb(admin_http_, "/pub", OnAdminPub, this);
  evhttp_set_cb(admin_http_, "broadcast", OnAdminBroadcast, this);
  evhttp_set_cb(admin_http_, "presence", OnAdminCheckPresence, this);
  evhttp_set_cb(admin_http_, "offmsg", OnAdminCheckOffMsg, this);
  evhttp_set_cb(admin_http_, "sub", OnAdminSub, this);
  evhttp_set_cb(admin_http_, "unsub", OnAdminUnsub, this);

  struct evhttp_bound_socket* sock;
  sock = evhttp_bind_socket_with_handle(admin_http_, FLAGS_admin_listen_ip.c_str(), FLAGS_admin_listen_port);
  CHECK(sock) << "bind address failed: " << strerror(errno);
  LOG(INFO) << "admin server listen on: " << FLAGS_admin_listen_ip << ":" << FLAGS_admin_listen_port;
  struct evconnlistener * listener = evhttp_bound_socket_get_listener(sock);
  evconnlistener_set_error_cb(listener, OnAcceptError);
}

void RouterServer::OnAcceptError(struct evconnlistener * listener, void * ctx) {
  LOG(ERROR) << "RouterServer::OnAcceptError";
}

void RouterServer::SendAllMessages(const UserID& uid) {
  Sid sid = FindSidByUid(uid);
  if (sid == INVALID_SID) {
    LOG(ERROR) << "unknow position of uid: " << uid;
    return;
  }
  storage_->GetMessage(uid,
      boost::bind(&RouterServer::OnGetMsgToSend, this, uid, _1));
}

void RouterServer::SendUserMsg(MessagePtr msg) {
  VLOG(5) << "RouterServer::SendUserMsg: " << msg->toStyledString();
  const string& uid = (*msg)["from"].asString();
  int seq = 0;
  Sid sid = INVALID_SID;
  UserInfoMap::iterator uiter = users_.find(uid);
  if (uiter != users_.end()) {
    seq = uiter->second.IncMaxSeq();
    sid = uiter->second.GetSSId();
  }
  storage_->SaveMessage(msg, seq,
      boost::bind(&RouterServer::OnSaveMessageDone, this, _1));
  if (sid == INVALID_SID) {
    LOG(INFO) << "uid " << uid << " is offline";
  } else {
    CHECK(size_t(sid) < session_clients_.size());
    try {
      (*msg)["seq"] = seq;
      Json::FastWriter writer;
      string content =  writer.write(*msg);
      session_clients_[sid]->Send(content);
    } catch (std::exception& e) {
      LOG(ERROR) << "SendUserMsg json exception: " << e.what();
    } catch (...) {
      LOG(ERROR) << "SendUserMsg unknow exception";
    }
  }
}

void RouterServer::SendChannelMsg(MessagePtr msg) {
  const string& uid = (*msg)["from"].asString();
  const string& cid = (*msg)["channel"].asString();
  VLOG(5) << "RouterServer::SendChannelMsg " << uid << ", " << cid;
  ChannelInfoMap::const_iterator iter = channels_.find(cid);
  if (iter == channels_.end()) {
    LOG(ERROR) << "channel not found: " << cid;
    return;
  }
  const set<string>& user_ids = iter->second.GetUsers();
  set<string>::const_iterator uit;
  for (uit = user_ids.begin(); uit != user_ids.end(); ++uit) {
    SendUserMsg(msg);
  }
}

void RouterServer::RemoveUserFromChannel(const UserInfo& user_info) {
  const set<string>& channel_ids = user_info.GetChannelIds();
  set<string>::const_iterator it;
  for (it = channel_ids.begin(); it != channel_ids.end(); ++it) {
    ChannelInfoMap::iterator cit = channels_.find(*it);
    if (cit != channels_.end()) {
      cit->second.RemoveUser(user_info.GetId());
      if (cit->second.GetUserCount() == 0) {
        channels_.erase(cit);
      }
    }
  }
}

void RouterServer::Subscribe(const UserID& uid, const ChannelID& cid) {
  VLOG(5) << "Subscribe: " << uid << ", "  << cid;
  UserInfoMap::iterator uit = users_.find(uid);
  if (uit == users_.end()) {
    LOG(ERROR) << "Subscribe failed: uid " << uid << " not found";
    return;
  }
  UserInfo& user = uit->second;
  ChannelInfoMap::iterator cit = channels_.find(user.GetId());
  if (cit == channels_.end()) {
    VLOG(5) << "add new channel: " << cid;
    ChannelInfo channel(cid);
    channel.AddUser(user.GetId());
    channels_.insert(make_pair(cid, channel));
  } else {
    cit->second.AddUser(user.GetId());
  }
  user.SubChannel(cid);
}

void RouterServer::Unsubscribe(const UserID& uid, const ChannelID& cid) {
  VLOG(5) << "Unsubscribe: " << uid << ", "  << cid;
  UserInfoMap::iterator uit = users_.find(uid);
  if (uit == users_.end()) {
    LOG(ERROR) << "Unsubscribe failed: uid " << uid << " not found";
  } else {
    uit->second.UnsubChannel(cid);
  }
  ChannelInfoMap::iterator cit = channels_.find(uit->second.GetId());
  if (cit == channels_.end()) {
    LOG(ERROR) << "Unsubscribe faied: cid " << cid << " not found";
  } else {
    cit->second.RemoveUser(uid);
    if (cit->second.GetUserCount() == 0) {
      LOG(INFO) << "delete channel from map: " << cid;
      channels_.erase(cid);
    }
  }
}

void RouterServer::UpdateUserAck(const UserID& uid, int seq) {
  UserInfoMap::iterator uit = users_.find(uid);
  if (uit == users_.end()) {
    LOG(ERROR) << "user not found: " << uid;
    return;
  }
  uit->second.SetLastAck(seq);
}

void RouterServer::OnSessionMsg(const HttpClient* client,
                                const string& msg,
                                void *ctx) {
  VLOG(5) << "RouterServer::OnSessionMsg: " << msg;
  RouterServer* self  = (RouterServer*)ctx;

  try {
    MessagePtr value_ptr(new Json::Value());
    Json::Value& value = *value_ptr;
    Json::Reader reader;
    CHECK(reader.parse(msg, value)) << "json parse failed, msg: " << msg;
    CHECK(value.isMember("type"));
    CHECK(value.isMember("from"));
    const string& type = value["type"].asString();
    const string& from = value["from"].asString();
    if(IS_MESSAGE(type)) {
      const string& to = value["to"].asString();
      if (IsUserId(to)) {
        self->SendUserMsg(value_ptr);
      } else {
        self->SendChannelMsg(value_ptr);
      }
    } else if(IS_LOGIN(type)) {
      self->LoginUser(from, client->GetOption().id);
      self->SendAllMessages(from);
    } else if(IS_LOGOUT(type)) {
      self->LogoutUser(from);
    } else if(IS_ACK(type)) {
      self->UpdateUserAck(from, value["seq"].asInt());
    } else if(IS_SUB(type)) {
      self->Subscribe(from, value["channel"].asString());
    } else if(IS_UNSUB(type)) {
      self->Unsubscribe(from, value["channel"].asString());
    } else if(IS_NOOP(type)) {
    } else {
      LOG(ERROR) << "unsupport message type: " << type;
    }
  } catch (std::exception& e) {
    CHECK(false) << "failed to process session message: " << e.what();
  } catch (...) {
    CHECK(false) << "unknow exception while process session message";
  }
}

void RouterServer::OnSessionRequestDone(const HttpClient* client,
                                        const string& resp,
                                        void *ctx) {
  VLOG(5) << "RouterServer::OnSessionRequestDone";
  RouterServer* self = (RouterServer*)ctx;
  CHECK(self);

  Sid sid = client->GetOption().id;
  CHECK(sid >= 0 && size_t(sid) < self->session_clients_.size());
  self->session_clients_[sid]->DelayRetry(HttpClient::OnRetry);
}

void RouterServer::OnGetMaxSeqDoneToLogin(const UserID uid, Sid sid, int seq) {
  VLOG(5) << "OnGetMaxSeqDoneToLogin: " << uid << ", " << sid << ", " << seq;
  UserInfo user(uid, sid);
  user.SetMaxSeq(seq);
  users_.insert(make_pair(uid, user));
  VLOG(5) << "users size: " << users_.size();
}

void RouterServer::LoginUser(const UserID& uid, Sid sid) {
  VLOG(5) << "RouterServer::LoginUser: " << uid << ", " << sid;
  storage_->GetMaxSeq(uid,
      boost::bind(&RouterServer::OnGetMaxSeqDoneToLogin, this, uid, sid, _1));
}

void RouterServer::OnUpdateAckDone(bool ok) {
  VLOG(5) << "OnUpdateAckDone: " << ok;
}

void RouterServer::LogoutUser(const UserID& uid) {
  VLOG(5) << "RouterServer::LogoutUser" << uid;
  UserInfoMap::iterator iter = users_.find(uid);
  if(iter == users_.end()) {
    LOG(ERROR) << "uid " << uid << " not found.";
    return;
  }
  storage_->UpdateAck(uid, iter->second.GetLastAck(),
      boost::bind(&RouterServer::OnUpdateAckDone, this, _1));
  RemoveUserFromChannel(iter->second);
  users_.erase(iter);
  VLOG(3) << "erase uid : " << uid;
  VLOG(5) << "users size: " << users_.size();
}

Sid RouterServer::FindSidByUid(const UserID& uid) const {
  UserInfoMap::const_iterator citer = users_.find(uid);
  if (citer == users_.end()) {
    return INVALID_SID;
  }
  return citer->second.GetSSId();
}

void RouterServer::OnSaveMessageDone(bool ok) {
  VLOG(5) << "OnSaveMessageDone: " << ok;
  if(!ok) {
    LOG(ERROR) << "OnSaveMessageDone failed";
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
    ReplyError(req);
    return;
  }

  struct evbuffer* input_buffer = evhttp_request_get_input_buffer(req);
  int len = evbuffer_get_length(input_buffer);
  VLOG(5) << "data length:" << len;
  const char * bufferstr = (const char*)evbuffer_pullup(input_buffer, len);
  if(bufferstr == NULL) {
    LOG(ERROR) << "evbuffer_pullup return null";
    ReplyError(req);
    return;
  }
  string content(bufferstr, len);
  MessagePtr msg(new Json::Value());
  Json::Reader reader;
  if (!reader.parse(content, *msg)) {
    LOG(ERROR) << "receive pub message with unexpected format: " << content;
    ReplyError(req);
    return;
  }
  self->SendUserMsg(msg);
  // TODO(qingfeng) maybe reply after save message done is better
  ReplyOK(req);
}

void RouterServer::OnAdminBroadcast(struct evhttp_request* req, void *ctx) {
  VLOG(5) << "RouterServer::OnAdminBroadcast";

}

void RouterServer::OnAdminCheckPresence(struct evhttp_request* req, void *ctx) {
  RouterServer * self = static_cast<RouterServer*>(ctx);
  VLOG(5) << "RouterServer::OnAdminCheckPresence";
  string response;
  string_format(response,  "{\"presence\": %d}", self->users_.size());
  SendReply(req, response.c_str(), response.size());
}

void RouterServer::OnAdminCheckOffMsg(struct evhttp_request* req, void *ctx) {
  RouterServer * self = static_cast<RouterServer*>(ctx);

  HttpQuery query(req);
  const char * uid = query.GetStr("uid", NULL);
  if (uid == NULL) {
    LOG(ERROR) << "uid not found";
    ReplyError(req);
    return;
  }
  self->storage_->GetMessage(uid, 0, -1,
      boost::bind(&RouterServer::OnGetMsgToReply,
          self, UserID(uid), req, _1));
}

void RouterServer::OnAdminSub(struct evhttp_request* req, void *ctx) {
  RouterServer * self = static_cast<RouterServer*>(ctx);

  HttpQuery query(req);
  UserID uid = query.GetStr("uid", "");
  ChannelID cid = query.GetStr("cid", "");
  if (uid.empty() || cid.empty()) {
    LOG(WARNING) << "uid and cid should not be empty";
    evhttp_send_reply(req, 410, "Invalid parameters", NULL);
  } else {
    self->Subscribe(uid, cid);
  }
}

void RouterServer::OnAdminUnsub(struct evhttp_request* req, void *ctx) {
  RouterServer * self = static_cast<RouterServer*>(ctx);

  HttpQuery query(req);
  UserID uid = query.GetStr("uid", "");
  ChannelID cid = query.GetStr("cid", "");
  if (uid.empty() || cid.empty()) {
    LOG(WARNING) << "uid and cid should not be empty";
    evhttp_send_reply(req, 410, "Invalid parameters", NULL);
  } else {
    self->Unsubscribe(uid, cid);
  }
}

void RouterServer::SendReply(struct evhttp_request* req, const char* content, size_t len) {
  struct evbuffer * output_buffer = evhttp_request_get_output_buffer(req);
  evbuffer_add(output_buffer, content, len);
  evhttp_send_reply(req, HTTP_OK, "OK", output_buffer);
}

void RouterServer::OnGetMsgToSend(UserID uid, MessageResult mr) {
  Sid sid = FindSidByUid(uid);
  if (sid == INVALID_SID) {
    LOG(INFO) << "uid " << uid << " is offline";
  } else {
    CHECK(size_t(sid) < session_clients_.size());
    if (mr.get() != NULL) {
      VLOG(5) << "OnGetMsgToSend: size = " << mr->size();
      for (int i = 0; i < mr->size(); ++i) {
        session_clients_[sid]->Send(mr->at(i));
      }
    } else {
      VLOG(3) << "no message to send";
    }
  }
}

void RouterServer::OnGetMsgToReply(UserID uid,
                                   struct evhttp_request* req,
                                   MessageResult mr) {
  VLOG(5) << "RouterServer::OnGetMsgToReply";
  Json::Value json(Json::arrayValue);
  if (mr.get() != NULL) {
    for (int i = 0; i < mr->size(); ++i) {
      json.append(mr->at(i));
    }
  }
  Json::FastWriter writer;
  string resp = writer.write(json);
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
