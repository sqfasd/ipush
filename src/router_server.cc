#include "src/router_server.h"

#include "deps/jsoncpp/include/json/json.h"
#include "deps/base/flags.h"
#include "deps/base/logging.h"
#include "deps/base/time.h"
#include "deps/base/string_util.h"
#include "deps/base/at_exit.h"
#include "deps/base/daemonizer.h"
#include "deps/limonp/StringUtil.hpp"
#include "src/http_query.h"

DEFINE_int32(admin_listen_port, 8100, "");
DEFINE_string(session_server_addrs, "127.0.0.1:9100", "");

#define IS_LOGIN(type) (type == "login")
#define IS_LOGOUT(type) (type == "logout")
#define IS_NOOP(type) (type == "noop")
#define IS_SUB(type) (type == "sub")
#define IS_UNSUB(type) (type == "unsub")
#define IS_MESSAGE(type) (type == "msg")
#define IS_CHANNEL_MSG(type) (type == "cmsg")
#define IS_ACK(type) (type == "ack")

namespace xcomet {

using Limonp::string_format;

const Sid INVALID_SID = -1;
const int DEFAULT_TIMER_INTERVAL = 1;

RouterServer::RouterServer(struct event_base* evbase)
    : evbase_(evbase),
      stats_(DEFAULT_TIMER_INTERVAL) {
  LOG(INFO) << "RouterServer construct";
  CHECK(evbase_);
}

RouterServer::~RouterServer() {
  LOG(INFO) << "RouterServer destruct";
	evhttp_free(admin_http_);
}

void RouterServer::Start() {
  InitStorage();
  ConnectSessionServers();
  InitAdminHttp();
  InitEventHandler();
  stats_.OnServerStart();
  event_base_dispatch(evbase_);
}

void RouterServer::ConnectSessionServers() {
  vector<string> addrs;
  SplitString(FLAGS_session_server_addrs, ',', &addrs);
  CHECK(addrs.size());
  CHECK(session_proxys_.empty());
  session_proxys_.resize(addrs.size());
  for (size_t i = 0; i < session_proxys_.size(); i++) {
    string host;
    int port;
    ParseIpPort(addrs[i], host, port);
    SessionProxy* proxy = new SessionProxy(evbase_, i);
    proxy->SetDisconnectCallback(
        boost::bind(&RouterServer::OnSessionProxyDisconnected, this, proxy));
    proxy->SetMessageCallback(
        boost::bind(&RouterServer::OnSessionMsg, this, proxy, _1, _2));
    proxy->SetHost(host);
    proxy->SetPort(port);
    session_proxys_[i].reset(proxy);
  }
  for (int i = 0; i < session_proxys_.size(); ++i) {
    session_proxys_[i]->StartConnect();
  }
}

void RouterServer::InitStorage() {
  LOG(INFO) << "InitStorage";
  RemoteStorageOption option;
  storage_.reset(new RemoteStorage(evbase_));
}

void RouterServer::InitAdminHttp() {
  admin_http_ = evhttp_new(evbase_);
  evhttp_set_cb(admin_http_, "/pub", OnAdminPub, this);
  evhttp_set_cb(admin_http_, "/broadcast", OnAdminBroadcast, this);
  evhttp_set_cb(admin_http_, "/presence", OnAdminCheckPresence, this);
  evhttp_set_cb(admin_http_, "/offmsg", OnAdminCheckOffMsg, this);
  evhttp_set_cb(admin_http_, "/sub", OnAdminSub, this);
  evhttp_set_cb(admin_http_, "/unsub", OnAdminUnsub, this);
  evhttp_set_cb(admin_http_, "/stats", OnAdminStats, this);

  const char* const BIND_IP = "0.0.0.0";
  struct evhttp_bound_socket* sock;
  sock = evhttp_bind_socket_with_handle(admin_http_,
                                        BIND_IP,
                                        FLAGS_admin_listen_port);
  CHECK(sock) << "bind address failed: " << strerror(errno);
  LOG(INFO) << "admin server listen on: "
            << BIND_IP << ":" << FLAGS_admin_listen_port;
  struct evconnlistener * listener = evhttp_bound_socket_get_listener(sock);
  evconnlistener_set_error_cb(listener, OnAcceptError);
}

void RouterServer::InitEventHandler() {
  struct event* sigint_event = NULL;
	sigint_event = evsignal_new(evbase_, SIGINT, OnSignal, this);
  CHECK(sigint_event && event_add(sigint_event, NULL) == 0)
      << "set SIGINT handler failed";

  struct event* sigterm_event = NULL;
	sigterm_event = evsignal_new(evbase_, SIGTERM, OnSignal, this);
  CHECK(sigterm_event && event_add(sigterm_event, NULL) == 0)
      << "set SIGTERM handler failed";

  struct event* timer_event = NULL;
	timer_event = event_new(evbase_, -1, EV_PERSIST, OnTimer, this);
	{
		struct timeval tv;
		tv.tv_sec = DEFAULT_TIMER_INTERVAL;
		tv.tv_usec = 0;
    CHECK(sigterm_event && event_add(timer_event, &tv) == 0)
        << "set timer handler failed";
	}
}

void RouterServer::OnAcceptError(struct evconnlistener * listener, void * ctx) {
  LOG(ERROR) << "RouterServer::OnAcceptError";
}

void RouterServer::SendAllMessages(const UserID& uid) {
  storage_->GetMessage(uid,
      boost::bind(&RouterServer::OnGetMsgToSend, this, uid, _1));
}

void RouterServer::SendUserMsg(MessagePtr msg) {
  VLOG(5) << "RouterServer::SendUserMsg: " << msg->toStyledString();
  if (!msg->isMember("to")) {
    LOG(ERROR) << "invalid message, no target";
    return;
  }
  const string& uid = (*msg)["to"].asString();
  int seq = 0;
  Sid sid = INVALID_SID;
  UserInfoMap::iterator uiter = users_.find(uid);
  if (uiter != users_.end()) {
    seq = uiter->second.IncMaxSeq();
    sid = uiter->second.GetSid();
  }
  storage_->SaveMessage(msg, seq,
      boost::bind(&RouterServer::OnSaveMessageDone, this, _1));

  if (uiter == users_.end() || !uiter->second.IsOnline()) {
    LOG(WARNING) << "user " << uid << " is not online";
  } else if (sid == INVALID_SID || !session_proxys_[sid]->IsConnected()) {
    LOG(WARNING) << "session server " << sid << " is not avaiable";
  } else {
    CHECK(size_t(sid) < session_proxys_.size());
    try {
      (*msg)["seq"] = seq;
      StringPtr data = Message::Serialize(msg);
      stats_.OnSend(*data);
      session_proxys_[sid]->SendData(*data);
    } catch (std::exception& e) {
      LOG(ERROR) << "SendUserMsg json exception: " << e.what();
    } catch (...) {
      LOG(ERROR) << "SendUserMsg unknow exception";
    }
  }
}

void RouterServer::OnGetChannelUsersToSend(
    const ChannelID cid,
    MessagePtr msg,
    UsersPtr users) {
  VLOG(5) << "OnGetChannelUsersToSend: cid = " << cid
          << "users.size = " << users->size();
  ChannelInfo channel(cid);
  for (int i = 0; i < users->size(); ++i) {
    channel.AddUser(users->at(i));
    (*msg)["to"] = users->at(i);
    SendUserMsg(msg);
  }
  channels_.insert(make_pair(cid, channel));
}

void RouterServer::SendChannelMsg(MessagePtr msg) {
  const ChannelID& cid = (*msg)["channel"].asString();
  const UserID& uid = (*msg)["from"].asString();
  VLOG(5) << "RouterServer::SendChannelMsg from " << uid << " to " << cid;
  ChannelInfoMap::const_iterator iter = channels_.find(cid);
  if (iter == channels_.end()) {
    storage_->GetChannelUsers(cid,
        boost::bind(&RouterServer::OnGetChannelUsersToSend,
            this, cid, msg, _1));
  } else {
    const set<string>& user_ids = iter->second.GetUsers();
    set<string>::const_iterator uit;
    for (uit = user_ids.begin(); uit != user_ids.end(); ++uit) {
      (*msg)["to"] = *uit;
      SendUserMsg(msg);
    }
  }
}

void RouterServer::OnAddUserToChannelDone(bool ok) {
  VLOG(5) << "OnAddUserToChannelDone: " << ok;
  if (!ok) {
    LOG(ERROR) << "add user to channel failed";
  }
}

void RouterServer::OnRemoveUserFromChannelDone(bool ok) {
  VLOG(5) << "OnRemoveUserFromChannelDone: " << ok;
  if (!ok) {
    LOG(ERROR) << "remove user from channel failed";
  }
}

void RouterServer::Subscribe(const UserID& uid, const ChannelID& cid) {
  VLOG(5) << "Subscribe: " << uid << ", "  << cid;
  ChannelInfoMap::iterator cit = channels_.find(cid);
  if (cit != channels_.end()) {
    cit->second.AddUser(uid);
  }
  storage_->AddUserToChannel(uid, cid,
      boost::bind(&RouterServer::OnAddUserToChannelDone, this, _1));
}

void RouterServer::Unsubscribe(const UserID& uid, const ChannelID& cid) {
  VLOG(5) << "Unsubscribe: " << uid << ", "  << cid;
  ChannelInfoMap::iterator cit = channels_.find(cid);
  if (cit != channels_.end()) {
    cit->second.RemoveUser(uid);
  }
  storage_->RemoveUserFromChannel(uid, cid,
      boost::bind(&RouterServer::OnRemoveUserFromChannelDone, this, _1));
}

void RouterServer::UpdateUserAck(const UserID& uid, int seq) {
  VLOG(5) << "UpdateUserAck: " << uid << ", " << seq;
  UserInfoMap::iterator uit = users_.find(uid);
  if (uit == users_.end()) {
    LOG(ERROR) << "user not found: " << uid;
    return;
  }
  uit->second.SetLastAck(seq);
  storage_->UpdateAck(uid, uit->second.GetLastAck(),
      boost::bind(&RouterServer::OnUpdateAckDone, this, _1));
}

void RouterServer::OnSessionMsg(SessionProxy* sp,
                                StringPtr raw_data,
                                MessagePtr msg) {
  VLOG(5) << "RouterServer::OnSessionMsg: " << msg->toStyledString();
  stats_.OnReceive(*raw_data);
  try {
    Json::Value& value = *msg;
    CHECK(value.isMember("type")) << "invalid message: " << *raw_data;
    const string& type = value["type"].asString();
    if(IS_MESSAGE(type)) {
        SendUserMsg(msg);
    } else if(IS_CHANNEL_MSG(type)) {
        SendChannelMsg(msg);
    } else if(IS_LOGIN(type)) {
      const string& from = value["from"].asString();
      LoginUser(from, sp->GetId());
    } else if(IS_LOGOUT(type)) {
      const string& from = value["from"].asString();
      LogoutUser(from);
    } else if(IS_ACK(type)) {
      const string& from = value["from"].asString();
      UpdateUserAck(from, value["seq"].asInt());
    } else if(IS_SUB(type)) {
      const string& from = value["from"].asString();
      Subscribe(from, value["channel"].asString());
    } else if(IS_UNSUB(type)) {
      const string& from = value["from"].asString();
      Unsubscribe(from, value["channel"].asString());
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

void RouterServer::OnSessionProxyDisconnected(SessionProxy* sp) {
  // TODO(qingfeng) don't waste time to erase user or traverse to set offline
  //                but when the proxy reconnected and the user haven't
  //                login on time, the router will send to the session server
  //                a message of the user that is not connected
  CHECK(sp != NULL);
  Sid sid = sp->GetId();
  CHECK(sid >= 0 && sid < session_proxys_.size());
  VLOG(5) << "session proxy " << sid << " disconnected";
  sp->Retry();
}

void RouterServer::OnGetMaxSeqDoneToLogin(const UserID uid, Sid sid, int seq) {
  LOG(INFO) << "OnGetMaxSeqDoneToLogin: " << uid << ", " << sid << ", " << seq;
  UserInfoMap::iterator uit = users_.find(uid);
  if (uit != users_.end()) {
    uit->second.SetOnline(true);
    uit->second.SetSid(sid);
    uit->second.SetMaxSeq(seq);
  } else {
    UserInfo user(uid, sid);
    user.SetMaxSeq(seq);
    users_.insert(make_pair(uid, user));
  }
  SendAllMessages(uid);
}

void RouterServer::LoginUser(const UserID& uid, Sid sid) {
  LOG(INFO) << "LoginUser: " << uid << ", " << sid;
  storage_->GetMaxSeq(uid,
      boost::bind(&RouterServer::OnGetMaxSeqDoneToLogin, this, uid, sid, _1));
}

void RouterServer::OnUpdateAckDone(bool ok) {
  VLOG(5) << "OnUpdateAckDone: " << ok;
}

void RouterServer::OnDeleteMessageDone(bool ok) {
  VLOG(5) << "OnDeleteMessageDone: " << ok;
}

void RouterServer::LogoutUser(const UserID& uid) {
  LOG(INFO) << "LogoutUser: " << uid;
  UserInfoMap::iterator iter = users_.find(uid);
  if(iter == users_.end()) {
    LOG(ERROR) << "uid " << uid << " not found when logout";
    return;
  }
  iter->second.SetOnline(false);
  storage_->DeleteMessage(uid,
      boost::bind(&RouterServer::OnDeleteMessageDone, this, _1));
}

Sid RouterServer::FindSidByUid(const UserID& uid) const {
  UserInfoMap::const_iterator citer = users_.find(uid);
  if (citer == users_.end() || !citer->second.IsOnline()) {
    return INVALID_SID;
  }
  return citer->second.GetSid();
}

void RouterServer::OnSaveMessageDone(bool ok) {
  VLOG(5) << "OnSaveMessageDone: " << ok;
  if(!ok) {
    LOG(ERROR) << "OnSaveMessageDone failed";
    return;
  }
}

void RouterServer::OnAdminPub(struct evhttp_request* req, void *ctx) {
  LOG(INFO) << "request: " << evhttp_request_get_uri(req);

  RouterServer * self = static_cast<RouterServer*>(ctx);
  VLOG(5) << "RouterServer::OnAdminPub";

  HttpQuery query(req);
  const char * to = query.GetStr("to", NULL);
  const char * from = query.GetStr("from", NULL);
  const char* channel = query.GetStr("channel", NULL);
  if ((to == NULL && channel == NULL) || from == NULL) {
    string error = "target or source id is invalid";
    LOG(ERROR) << error;
    ReplyError(req, HTTP_BADREQUEST, error);
    return;
  }

  struct evbuffer* input_buffer = evhttp_request_get_input_buffer(req);
  int len = evbuffer_get_length(input_buffer);
  VLOG(5) << "data length:" << len;
  const char * bufferstr = (const char*)evbuffer_pullup(input_buffer, len);
  if(bufferstr == NULL) {
    string error = "body cannot be empty";
    LOG(ERROR) << error;
    ReplyError(req, HTTP_BADREQUEST, error);
    return;
  }

  // TODO(qingfeng) maybe reply after save message done is better
  MessagePtr msg(new Json::Value());
  if (to != NULL) {
    (*msg)["type"] = "msg";
    (*msg)["from"] = from;
    (*msg)["to"] = to;
    (*msg)["body"] = string(bufferstr, len);
    self->SendUserMsg(msg);
  } else {
    CHECK(channel != NULL);
    (*msg)["type"] = "cmsg";
    (*msg)["from"] = from;
    (*msg)["channel"] = channel;
    (*msg)["body"] = string(bufferstr, len);
    self->SendChannelMsg(msg);
  }
  ReplyOK(req);
}

void RouterServer::OnAdminBroadcast(struct evhttp_request* req, void *ctx) {
  LOG(INFO) << "request: " << evhttp_request_get_uri(req);
}

void RouterServer::OnAdminCheckPresence(struct evhttp_request* req, void *ctx) {
  LOG(INFO) << "request: " << evhttp_request_get_uri(req);

  RouterServer * self = static_cast<RouterServer*>(ctx);
  VLOG(5) << "RouterServer::OnAdminCheckPresence";
  string response;
  string_format(response,  "{\"presence\": %d}", self->users_.size());
  ReplyOK(req, response);
}

void RouterServer::OnAdminCheckOffMsg(struct evhttp_request* req, void *ctx) {
  LOG(INFO) << "request: " << evhttp_request_get_uri(req);

  RouterServer * self = static_cast<RouterServer*>(ctx);
  HttpQuery query(req);
  const char * uid = query.GetStr("uid", NULL);
  if (uid == NULL) {
    string error = "target id is invalid";
    LOG(ERROR) << error;
    ReplyError(req, HTTP_BADREQUEST, error);
    return;
  }
  self->storage_->GetMessage(uid,
      boost::bind(&RouterServer::OnGetMsgToReply,
          self, UserID(uid), req, _1));
}

void RouterServer::OnAdminSub(struct evhttp_request* req, void *ctx) {
  LOG(INFO) << "request: " << evhttp_request_get_uri(req);

  RouterServer * self = static_cast<RouterServer*>(ctx);
  HttpQuery query(req);
  UserID uid = query.GetStr("uid", "");
  ChannelID cid = query.GetStr("cid", "");
  if (uid.empty() || cid.empty()) {
    string error = "uid and cid should not be empty";
    LOG(ERROR) << error;
    ReplyError(req, HTTP_BADREQUEST, error);
  } else {
    // TODO(qingfeng) if failed to subscribe, reply error
    self->Subscribe(uid, cid);
    ReplyOK(req);
  }
}

void RouterServer::OnAdminUnsub(struct evhttp_request* req, void *ctx) {
  LOG(INFO) << "request: " << evhttp_request_get_uri(req);

  RouterServer * self = static_cast<RouterServer*>(ctx);
  HttpQuery query(req);
  UserID uid = query.GetStr("uid", "");
  ChannelID cid = query.GetStr("cid", "");
  if (uid.empty() || cid.empty()) {
    string error = "uid and cid should not be empty";
    LOG(ERROR) << error;
    ReplyError(req, HTTP_BADREQUEST, error);
  } else {
    self->Unsubscribe(uid, cid);
    ReplyOK(req);
  }
}

void RouterServer::OnGetMsgToSend(UserID uid, MessageResult mr) {
  LOG(INFO) << "OnGetMsgToSend msg number = " << mr->size() / 2
            << ", uid = " << uid;
  Sid sid = FindSidByUid(uid);
  if (sid == INVALID_SID) {
    LOG(WARNING) << "uid " << uid << " is offline";
  } else {
    CHECK(size_t(sid) < session_proxys_.size());
    if (mr.get() != NULL) {
      if (!session_proxys_[sid]->IsConnected()) {
        LOG(WARNING) << "session proxy is not avaiable";
        return;
      }
      for (int i = 1; i < mr->size(); i+=2) {
        stats_.OnSend(mr->at(i));
        session_proxys_[sid]->SendData(mr->at(i));
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
    for (int i = 1; i < mr->size(); i+=2) {
      json.append(mr->at(i));
    }
  }
  ReplyOK(req, json.toStyledString());
}

void RouterServer::OnTimer(evutil_socket_t sig, short events, void *ctx) {
  VLOG(5) << "OnTimer";
  RouterServer* self = static_cast<RouterServer*>(ctx);
  self->stats_.OnTimer(self->users_.size());
}

void RouterServer::OnDestroy() {
  for (int i = 0; i < session_proxys_.size(); ++i) {
    session_proxys_[i]->StopRetry();
    session_proxys_[i]->WaitForClose();
    session_proxys_[i].reset();
  }
  storage_.reset();
}

void RouterServer::OnSignal(evutil_socket_t sig, short events, void *ctx) {
  LOG(INFO) << "RouterServer::OnSignal: " << sig << ", " << events;
  RouterServer* self = static_cast<RouterServer*>(ctx);
  self->OnDestroy();
  event_base_loopbreak(self->evbase_);
  LOG(INFO) << "main loop break";
}

void RouterServer::OnAdminStats(struct evhttp_request* req, void *ctx) {
  LOG(INFO) << "request: " << evhttp_request_get_uri(req);

  RouterServer* self = static_cast<RouterServer*>(ctx);
  Json::Value response;
  Json::Value& result = response["result"];
  self->stats_.GetReport(result);
  ReplyOK(req, response.toStyledString());
}

} // namespace xcomet

int main(int argc, char ** argv) {
  base::AtExitManager at_exit;
  base::ParseCommandLineFlags(&argc, &argv, false);
  base::daemonize();
  if (FLAGS_flagfile.empty()) {
    LOG(WARNING) << "not using --flagfile option !";
  }
  LOG(INFO) << "command line options\n" << base::CommandlineFlagsIntoString();

  struct event_base* evbase = event_base_new();
  {
    xcomet::RouterServer server(evbase);
    server.Start();
  }
  event_base_free(evbase);
  return EXIT_SUCCESS;
}
