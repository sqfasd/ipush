#include "src/session_server.h"

#include "deps/jsoncpp/include/json/json.h"
#include "deps/base/logging.h"
#include "deps/base/flags.h"
#include "deps/base/file.h"
#include "deps/base/string_util.h"
#include "deps/base/daemonizer.h"
#include "src/loop_executor.h"
#include "src/inmemory_storage.h"

DEFINE_int32(client_listen_port, 9000, "");
DEFINE_int32(admin_listen_port, 9001, "");
DEFINE_int32(poll_timeout_sec, 1800, "");
DEFINE_int32(timer_interval_sec, 1, "");
DEFINE_bool(is_server_heartbeat, false, "");
DEFINE_int32(peer_id, 0, "");
DEFINE_string(peers_ip, "192.168.2.3", "LAN ip");
DEFINE_string(peers_address, "192.168.2.3:9000", "public client address");
DEFINE_string(peers_admin_address, "192.168.2.3:9001", "admin peers address");
DEFINE_bool(check_offline_msg_on_login, true, "");

const bool CHECK_SHARD = true;
const bool NO_CHECK_SHARD = false;

#define CHECK_HTTP_GET()\
  do {\
    if(evhttp_request_get_command(req) != EVHTTP_REQ_GET) {\
      ReplyError(req, HTTP_BADMETHOD);\
      return;\
    }\
  } while(0)

#define CHECK_HTTP_POST()\
  do {\
    if(evhttp_request_get_command(req) != EVHTTP_REQ_POST) {\
      ReplyError(req, HTTP_BADMETHOD);\
      return;\
    }\
  } while(0)

#define CHECK_REDIRECT_CLIENT(uid)\
  do {\
    int shard_id = GetShardId(uid);\
    if (shard_id != peer_id_) {\
      VLOG(3) << "redirect to shard " << shard_id;\
      ReplyRedirect(req, peers_[shard_id].public_addr);\
      return;\
    }\
  } while(0)

#define CHECK_REDIRECT_ADMIN(uid)\
  do {\
    int shard_id = GetShardId(uid);\
    if (shard_id != peer_id_) {\
      VLOG(3) << "redirect to shard " << shard_id;\
      ReplyRedirect(req, peers_[shard_id].admin_addr);\
      return;\
    }\
  } while(0)

namespace xcomet {

static void ConnectHandler(struct evhttp_request* req, void* ctx) {
  LOG(INFO) << "request: " << evhttp_request_get_uri(req);
  SessionServer* server = static_cast<SessionServer*>(ctx);
  server->Connect(req);
}

static void DisconnectHandler(struct evhttp_request* req, void* ctx) {
  LOG(INFO) << "request: " << evhttp_request_get_uri(req);
  SessionServer* server = static_cast<SessionServer*>(ctx);
  server->Disconnect(req);
}

static void PubHandler(struct evhttp_request* req, void* ctx) {
  LOG(INFO) << "request: " << evhttp_request_get_uri(req);
  SessionServer* server = static_cast<SessionServer*>(ctx);
  server->Pub(req);
}

static void BroadcastHandler(struct evhttp_request* req, void* ctx) {
  LOG(INFO) << "request: " << evhttp_request_get_uri(req);
  SessionServer* server = static_cast<SessionServer*>(ctx);
  server->Broadcast(req);
}

static void StatsHandler(struct evhttp_request* req, void* ctx) {
  LOG(INFO) << "request: " << evhttp_request_get_uri(req);
  SessionServer* server = static_cast<SessionServer*>(ctx);
  server->Stats(req);
}

static void SubHandler(struct evhttp_request* req, void* ctx) {
  LOG(INFO) << "request: " << evhttp_request_get_uri(req);
  SessionServer* server = static_cast<SessionServer*>(ctx);
  server->Sub(req);
}

static void UnsubHandler(struct evhttp_request* req, void* ctx) {
  LOG(INFO) << "request: " << evhttp_request_get_uri(req);
  SessionServer* server = static_cast<SessionServer*>(ctx);
  server->Unsub(req);
}

static void MsgHandler(struct evhttp_request* req, void* ctx) {
  LOG(INFO) << "request: " << evhttp_request_get_uri(req);
  SessionServer* server = static_cast<SessionServer*>(ctx);
  server->Msg(req);
}

static void ShardHandler(struct evhttp_request* req, void* ctx) {
  LOG(INFO) << "request: " << evhttp_request_get_uri(req);
  SessionServer* server = static_cast<SessionServer*>(ctx);
  server->Shard(req);
}

static void AcceptErrorHandler(struct evconnlistener* listener, void* ptr) {
  LOG(ERROR) << "AcceptErrorHandler";
}

static void SignalHandler(evutil_socket_t sig, short events, void* ctx) {
  LOG(INFO) << "SignalHandler: " << sig << ", " << events;
  SessionServer* server = static_cast<SessionServer*>(ctx);
  server->Stop();
}

static void TimerHandler(evutil_socket_t sig, short events, void *ctx) {
  SessionServer* server = static_cast<SessionServer*>(ctx);
  server->OnTimer();
}

struct SessionServerPrivate {
  struct event_base* evbase;
  struct evhttp* client_http;
  struct evhttp* admin_http;
  struct event* sigterm_event;
  struct event* sigint_event;
  struct event* timer_event;

  SessionServerPrivate()
      : evbase(NULL),
        client_http(NULL),
        admin_http(NULL),
        sigterm_event(NULL),
        sigint_event(NULL),
        timer_event(NULL) {
    evbase = event_base_new();
    CHECK(evbase) << "create evbase failed";
  }
  ~SessionServerPrivate() {
    if (timer_event) event_free(timer_event);
    if (sigterm_event) event_free(sigterm_event);
    if (sigint_event) event_free(sigint_event);
    if (client_http) evhttp_free(client_http);
    if (admin_http) evhttp_free(admin_http);
    if (evbase) event_base_free(evbase);
  }
};

SessionServer::SessionServer()
    : client_listen_port_(FLAGS_client_listen_port),
      admin_listen_port_(FLAGS_admin_listen_port),
      timeout_counter_(FLAGS_poll_timeout_sec / FLAGS_timer_interval_sec),
      timeout_queue_(timeout_counter_),
      stats_(FLAGS_timer_interval_sec),
      p_(new SessionServerPrivate()),
      storage_(new InMemoryStorage()),
      peer_id_(FLAGS_peer_id),
      auth_(new Auth()) {
  vector<string> peers_ip;
  SplitString(FLAGS_peers_ip, ',', &peers_ip);
  vector<string> peers_address;
  SplitString(FLAGS_peers_address, ',', &peers_address);
  vector<string> peers_admin_address;
  SplitString(FLAGS_peers_admin_address, ',', &peers_admin_address);
  CHECK(peers_ip.size() > 0);
  CHECK(peers_ip.size() == peers_address.size());
  CHECK(peers_ip.size() == peers_admin_address.size());
  for (int i = 0; i < peers_ip.size(); ++i) {
    PeerInfo info;
    info.id = i;
    info.ip = peers_ip[i];
    info.public_addr = peers_address[i];
    info.admin_addr = peers_admin_address[i];
    peers_.push_back(info);
  }
  CHECK(peer_id_ < peers_ip.size());
  cluster_.reset(new Peer(peer_id_, peers_));
  sharding_.reset(new Sharding<PeerInfo>(peers_));
}

SessionServer::~SessionServer() {
  LOG(INFO) << "~SessionServer";
}

void SessionServer::Start() {
  SetupClientHandler();
  SetupAdminHandler();
  SetupEventHandler();
  OnStart();
	event_base_dispatch(p_->evbase);
}

void SessionServer::Stop() {
  OnStop();
  event_base_loopbreak(p_->evbase);
}

void SessionServer::OnStart() {
  xcomet::LoopExecutor::Init(p_->evbase);
  stats_.OnServerStart();
  cluster_->Start();
  cluster_->SetMessageCallback(bind(&SessionServer::OnPeerMessage, this, _1));
}

void SessionServer::OnStop() {
  xcomet::LoopExecutor::Destroy();
  cluster_->Stop();
}

// /connect?uid=123&token=ABCDE&type=1|2
void SessionServer::Connect(struct evhttp_request* req) {
  CHECK_HTTP_GET();
  HttpQuery query(req);
  string uid = query.GetStr("uid", "");
  string password = query.GetStr("password", "");
  int type = query.GetInt("type", 1);
  if (uid.empty() || password.empty()) {
    ReplyError(req, HTTP_BADREQUEST, "uid or password should not empty");
    return;
  }

  CHECK_REDIRECT_CLIENT(uid);

  auth_->Authenticate(uid, password, [req, uid, type, this](ErrorPtr err,
                                                            bool ok) {
    if (err.get() != NULL || !ok) {
      ReplyError(req, HTTP_BADREQUEST, "authentication failed"); 
      return;
    }
    UserPtr user(new User(uid, type, req, *this));
    UserMap::iterator iter = users_.find(uid);
    if (iter == users_.end()) {
      LOG(INFO) << "login user: " << uid;
    } else {
      timeout_queue_.RemoveUser(iter->second.get());
      LOG(INFO) << "relogin user: " << uid;
    }
    users_[uid] = user;
    timeout_queue_.PushUserBack(user.get());

    UserInfoMap::iterator info_iter = user_infos_.find(uid);
    if (info_iter == user_infos_.end()) {
      user_infos_.insert(make_pair(uid, UserInfo(uid)));
    }

    if (!FLAGS_check_offline_msg_on_login) {
      return;
    }

    storage_->GetMessage(uid, [uid, this](ErrorPtr error, MessageDataSet m) {
      if (error.get() != NULL) {
        LOG(ERROR) << "GetMessage failed: " << *error;
        return;
      }
      if (m.get() != NULL && m->size() > 0) {
        UserMap::iterator uit = users_.find(uid);
        if (uit == users_.end()) {
          LOG(WARNING) << "user offline after get offline messages: " << uid;
          return;
        }
        for (int i = 0; i < m->size(); ++i) {
          uit->second->Send(m->at(i));
        }
      } else {
        VLOG(3) << "no offline message for this user: " << uid;
      }
    });
  });
}

void SessionServer::SendUserMsg(Message& msg, int64 ttl, bool check_shard) {
  VLOG(5) << "SendUserMsg: " << msg;
  if (!msg.HasTo()) {
    LOG(ERROR) << "invalid message, no target: " << msg;
    return;
  }
  const string& uid = msg.To();
  if (check_shard) {
    int shard_id = GetShardId(uid);
    if (shard_id != peer_id_) {
      VLOG(4) << "send to peer " << shard_id << ": " << msg;
      cluster_->Send(shard_id, *(Message::Serialize(msg)));
      return;
    }
  }
  SendSave(uid, msg, ttl);
}

void SessionServer::SendSave(const string& uid, Message& msg, int64 ttl) {
  auto info_it = user_infos_.find(uid);
  if (info_it == user_infos_.end()) {
    info_it = user_infos_.insert(make_pair(uid, UserInfo(uid))).first;
  }
  CHECK(info_it != user_infos_.end());
  if (info_it->second.GetMaxSeq() == -1) {
    storage_->GetMaxSeq(uid, [this, info_it, msg, ttl](ErrorPtr error,
                                                       int seq) {
      if (error.get() != NULL) {
        LOG(ERROR) << "GetMaxSeq failed: " << *error;
        return;
      }
      CHECK(seq >= 0);
      info_it->second.SetMaxSeq(seq);
      ((Message&)msg).SetSeq(info_it->second.IncMaxSeq());
      DoSendSave(msg, ttl);
    });
  } else {
    ((Message&)msg).SetSeq(info_it->second.IncMaxSeq());
    DoSendSave(msg, ttl);
  }
}

void SessionServer::DoSendSave(const Message& msg, int64 ttl) {
  StringPtr data = Message::Serialize(msg);
  const string& uid = msg.To();
  if (data->empty()) {
    LOG(ERROR) << "serialize failed: " << msg;
    return;
  }
  auto user_it = users_.find(msg.To());
  if (user_it != users_.end()) {
    stats_.OnSend(*data);
    user_it->second->Send(*data);
  }
  storage_->SaveMessage(data, uid, ttl, [](ErrorPtr error_save) {
    if (error_save.get() != NULL) {
      LOG(ERROR) << "SaveMessage failed: " << *error_save;
      return;
    }
    VLOG(5) << "SaveMessage done";
  });
}

void SessionServer::SendChannelMsg(Message& msg, int64 ttl) {
  const string cid = msg.Channel();
  const string uid = msg.From();
  VLOG(5) << "SendChannelMsg from " << uid << " to " << cid;
  ChannelInfoMap::const_iterator iter = channels_.find(cid);
  if (iter == channels_.end()) {
    storage_->GetChannelUsers(cid, [cid, msg, ttl, this](ErrorPtr error,
                                                         UserResultSet u) {
      if (error.get() != NULL) {
        LOG(ERROR) << "GetChannelUsers failed: " << *error;
        return;
      }
      if (u.get() == NULL) {
        LOG(WARNING) << "no user in this channel: " << cid;
        return;
      }
      ChannelInfo channel(cid);
      for (int i = 0; i < u->size(); ++i) {
        const string& cuser = u->at(i);
        channel.AddUser(cuser);
        Message copy = msg.Clone();
        copy.SetTo(cuser);
        if (CheckShard(cuser)) {
          SendUserMsg(copy, ttl, NO_CHECK_SHARD);
        }
      }
      channels_.insert(make_pair(cid, channel));
    });
  } else {
    const auto& user_ids = iter->second.GetUsers();
    auto uit = user_ids.begin();
    for (; uit != user_ids.end(); ++uit) {
      Message copy = msg.Clone();
      copy.SetTo(*uit);
      if (CheckShard(*uit)) {
        SendUserMsg(copy, ttl, NO_CHECK_SHARD);
      }
    }
  }
}

void SessionServer::Pub(struct evhttp_request* req) {
  CHECK_HTTP_POST();

  HttpQuery query(req);
  const char * to = query.GetStr("to", NULL);
  const char * from = query.GetStr("from", NULL);
  const char* channel = query.GetStr("channel", NULL);
  int64 ttl = query.GetInt("ttl", NO_EXPIRE);
  if ((to == NULL && channel == NULL) || from == NULL) {
    ReplyError(req, HTTP_BADREQUEST, "target or source id is invalid");
    return;
  }

  struct evbuffer* input_buffer = evhttp_request_get_input_buffer(req);
  int len = evbuffer_get_length(input_buffer);
  VLOG(5) << "data length:" << len;
  const char * bufferstr = (const char*)evbuffer_pullup(input_buffer, len);
  if(bufferstr == NULL) {
    ReplyError(req, HTTP_BADREQUEST, "body cannot be empty");
    return;
  }

  stats_.OnPubRequest();

  // TODO(qingfeng) maybe reply after save message done is better
  Message msg;
  if (to != NULL) {
    msg.SetType(Message::T_MESSAGE);
    msg.SetFrom(from);
    msg.SetTo(to);
    msg.SetBody(string(bufferstr, len));
    SendUserMsg(msg, ttl, CHECK_SHARD);
  } else {
    CHECK(channel != NULL);
    msg.SetType(Message::T_CHANNEL_MESSAGE);
    msg.SetFrom(from);
    msg.SetChannel(channel);
    msg.SetBody(string(bufferstr, len));
    SendChannelMsg(msg, ttl);
    cluster_->Broadcast(*(Message::Serialize(msg)));
  }
  ReplyOK(req);
}

void SessionServer::Disconnect(struct evhttp_request* req) {
  CHECK_HTTP_GET();

  HttpQuery query(req);
  string uid = query.GetStr("uid", "");
  if (uid.empty()) {
    ReplyError(req, HTTP_BADREQUEST, "uid is empty");
    return;
  }

  CHECK_REDIRECT_ADMIN(uid);

  UserMap::iterator uit = users_.find(uid);
  if (uit == users_.end()) {
    ReplyError(req, HTTP_INTERNAL, "user not found: " + uid);
  } else {
    uit->second->Close();
    ReplyOK(req);
  }
}

void SessionServer::Sub(struct evhttp_request* req) {
  CHECK_HTTP_GET();

  HttpQuery query(req);
  string uid = query.GetStr("uid", "");
  string cid = query.GetStr("cid", "");
  if (uid.empty() || cid.empty()) {
    ReplyError(req, HTTP_BADREQUEST, "uid and cid should not be empty");
  } else {
    // TODO(qingfeng) if failed to subscribe, reply error
    int shard_id = GetShardId(uid);
    if (shard_id == peer_id_) {
      Subscribe(uid, cid);
    } else {
      Message msg;
      msg.SetType(Message::T_SUBSCRIBE);
      msg.SetUser(uid);
      msg.SetChannel(cid);
      cluster_->Send(shard_id, *(Message::Serialize(msg)));
    }
    ReplyOK(req);
  }
}

void SessionServer::Subscribe(const string& uid, const string& cid) {
  VLOG(5) << "Subscribe: " << uid << ", "  << cid;
  ChannelInfoMap::iterator cit = channels_.find(cid);
  if (cit != channels_.end()) {
    cit->second.AddUser(uid);
  }
  storage_->AddUserToChannel(uid, cid, [](ErrorPtr error) {
    if (error.get() != NULL) {
      LOG(ERROR) << "AddUserToChannel failed: " << *error;
      return;
    }
    VLOG(5) << "AddUserToChannel done";
  });
}

void SessionServer::Unsubscribe(const string& uid, const string& cid) {
  VLOG(5) << "Unsubscribe: " << uid << ", "  << cid;
  ChannelInfoMap::iterator cit = channels_.find(cid);
  if (cit != channels_.end()) {
    cit->second.RemoveUser(uid);
  }
  storage_->RemoveUserFromChannel(uid, cid, [](ErrorPtr error) {
    if (error.get() != NULL) {
      LOG(ERROR) << "RemoveUserFromChannel failed: " << *error;
      return;
    }
    VLOG(5) << "RemoveUserFromChannel done";;
  });
}

void SessionServer::Unsub(struct evhttp_request* req) {
  CHECK_HTTP_GET();

  HttpQuery query(req);
  string uid = query.GetStr("uid", "");
  string cid = query.GetStr("cid", "");
  if (uid.empty() || cid.empty()) {
    ReplyError(req, HTTP_BADREQUEST, "uid and cid should not be empty");
  } else {
    int shard_id = GetShardId(uid);
    if (shard_id == peer_id_) {
      Unsubscribe(uid, cid);
    } else {
      Message msg;
      msg.SetType(Message::T_SUBSCRIBE);
      msg.SetUser(uid);
      msg.SetChannel(cid);
      cluster_->Send(shard_id, *(Message::Serialize(msg)));
    }
    ReplyOK(req);
  }
}

void SessionServer::Msg(struct evhttp_request* req) {
  CHECK_HTTP_GET();

  HttpQuery query(req);
  const char * uid = query.GetStr("uid", NULL);
  if (uid == NULL) {
    ReplyError(req, HTTP_BADREQUEST, "target id is invalid");
    return;
  }

  CHECK_REDIRECT_ADMIN(uid);

  storage_->GetMessage(uid, [req](ErrorPtr error, MessageDataSet m) {
    if (error.get() != NULL) {
      LOG(ERROR) << "GetMessage failed: " << *error;
      ReplyError(req, HTTP_INTERNAL, *error);
      return;
    }
    Json::Value json(Json::arrayValue);
    if (m.get() != NULL) {
      for (int i = 0; i < m->size(); ++i) {
        json.append(m->at(i));
      }
    }
    ReplyOK(req, json.toStyledString());
  });
}

void SessionServer::Shard(struct evhttp_request* req) {
  CHECK_HTTP_GET();

  HttpQuery query(req);
  const char* uid = query.GetStr("uid", NULL);
  if (uid == NULL) {
    ReplyError(req, HTTP_BADREQUEST, "uid is needed");
    return;
  }
  Json::Value resp;
  resp["result"] = (*sharding_)[uid].public_addr;
  ReplyOK(req, resp.toStyledString());
}

// /broadcast?content=hello
void SessionServer::Broadcast(struct evhttp_request* req) {
  // TODO(qingfeng) is broadcast needed?
}

void SessionServer::OnTimer() {
  VLOG(5) << "OnTimer";
  stats_.OnTimer(users_.size());
  DLinkedList<User*> timeout_users = timeout_queue_.GetFront();
  DLinkedList<User*>::Iterator it = timeout_users.GetIterator();
  if (FLAGS_is_server_heartbeat) {
    while (User* user = it.Next()) {
      if (user->GetType() == User::COMET_TYPE_POLLING) {
        user->Close();
      } else {
        user->SendHeartbeat();
      }
    }
  } else { // for performance consider, don't merge the two loop
    while (User* user = it.Next()) {
      user->Close();
    }
  }

  timeout_queue_.IncHead();

  function<void ()> task;
  while (task_queue_.TryPop(task)) {
    task();
  }
}

void SessionServer::RunInNextTick(function<void ()> fn) {
  task_queue_.Push(fn);
}

void SessionServer::UpdateUserAck(const string& uid, int ack) {
  VLOG(5) << "UpdateUserAck: " << uid << ", " << ack;
  UserInfoMap::iterator uit = user_infos_.find(uid);
  if (uit == user_infos_.end()) {
    LOG(ERROR) << "user not found: " << uid;
    return;
  }
  uit->second.SetLastAck(ack);
  storage_->UpdateAck(uid, uit->second.GetLastAck(), [](ErrorPtr error) {
    if (error.get() != NULL) {
      LOG(ERROR) << "UpdateAck failed: " << *error;
      return;
    }
    VLOG(5) << "UpdateAck done";
  });
}

void SessionServer::OnUserMessage(const string& from, StringPtr data) {
  VLOG(4) << "OnUserMessage: " << from << ": " << *data;
  stats_.OnReceive(*data);

  try {
    Message msg = Message::Unserialize(data);
    HandleMessage(msg);
  } catch (std::exception& e) {
    LOG(ERROR) << "json exception: " << e.what() << ", msg = " << *data;
  } catch (...) {
    LOG(ERROR) << "unknow exception for user msg: " << *data;
  }

  UserMap::iterator uit = users_.find(from);
  if (uit == users_.end()) {
    LOG(WARNING) << "user not found: " << from;
  } else {
    timeout_queue_.PushUserBack(uit->second.get());
  }
}

void SessionServer::HandleMessage(Message& msg) {
  if (!msg.HasType()) {
    LOG(ERROR) << "invalid message without type: " << msg;
    return;
  }
  switch(msg.Type()) {
    case Message::T_MESSAGE:
      SendUserMsg(msg, NO_EXPIRE, CHECK_SHARD);
      break;
    case Message::T_CHANNEL_MESSAGE:
      SendChannelMsg(msg, NO_EXPIRE);
      break;
    case Message::T_ACK:
      UpdateUserAck(msg.From(), msg.Seq());
      break;
    case Message::T_SUBSCRIBE:
      Subscribe(msg.User(), msg.Channel());
      break;
    case Message::T_UNSUBSCRIBE:
      Unsubscribe(msg.User(), msg.Channel());
      break;
    case Message::T_HEARTBEAT:
      break;
    default:
      LOG(ERROR) << "unsupport message type: " << msg;
      break;
  }
}

void SessionServer::OnPeerMessage(PeerMessagePtr pmsg) {
  VLOG(3) << "OnPeerMessage: " << *pmsg;
  try {
    Message msg = Message::UnserializeString(pmsg->content);
    int64 ttl = msg.TTL();
    switch (msg.Type()) {
      case Message::T_MESSAGE: {
        const string& user = msg.To();
        if (CheckShard(user)) {
          LoopExecutor::RunInMainLoop(
              bind(&SessionServer::SendUserMsg, this, msg, ttl, NO_CHECK_SHARD));
        } else {
          LOG(ERROR) << "wrong shard, user: " << user
                     << ", pmsg: " << *pmsg;
        }
        break;
      }
      case Message::T_CHANNEL_MESSAGE: {
        LoopExecutor::RunInMainLoop(
            bind(&SessionServer::SendChannelMsg, this, msg, ttl));
        break;
      }
      case Message::T_SUBSCRIBE: {
        string uid = msg.User();
        string cid = msg.Channel();
        LoopExecutor::RunInMainLoop(
            bind(&SessionServer::Subscribe, this, uid, cid));
        break;
      }
      case Message::T_UNSUBSCRIBE: {
        string uid = msg.User();
        string cid = msg.Channel();
        LoopExecutor::RunInMainLoop(
            bind(&SessionServer::Unsubscribe, this, uid, cid));
        break;
      }
      default:
        LOG(ERROR) << "unexpected peer message type: " << *pmsg;
        break;
    }
  } catch (std::exception& e) {
    LOG(ERROR) << "json exception: " << e.what() << ", msg = " << *pmsg;
  } catch (...) {
    LOG(ERROR) << "unknow exception for peer msg: " << *pmsg;
  }
}

bool SessionServer::CheckShard(const string& user) {
  return (*sharding_)[user].id == peer_id_;
}

int SessionServer::GetShardId(const string& user) {
  return (*sharding_)[user].id;
}

void SessionServer::OnUserDisconnect(User* user) {
  const string& uid = user->GetId();
  LOG(INFO) << "OnUserDisconnect: " << uid;
  timeout_queue_.RemoveUser(user);
  users_.erase(uid);
}

void SessionServer::Stats(struct evhttp_request* req) {
  // TODO(qingfeng) check pretty json format parameter
  Json::Value response;
  Json::Value& result = response["result"];
  stats_.GetReport(result);
  ReplyOK(req, response.toStyledString());
}

void SessionServer::SetupClientHandler() {
  p_->client_http = evhttp_new(p_->evbase);
  CHECK(p_->client_http) << "create client http handle failed";
  evhttp_set_cb(p_->client_http, "/connect", ConnectHandler, this);

  struct evhttp_bound_socket* sock = NULL;
  sock = evhttp_bind_socket_with_handle(p_->client_http,
                                        "0.0.0.0",
                                        client_listen_port_);
  CHECK(sock) << "bind address failed: " << strerror(errno);

  LOG(INFO) << "clinet server listen on " << client_listen_port_;

  struct evconnlistener *listener = evhttp_bound_socket_get_listener(sock);
  evconnlistener_set_error_cb(listener, AcceptErrorHandler);
}

void SessionServer::SetupAdminHandler() {
  p_->admin_http = evhttp_new(p_->evbase);
  CHECK(p_->admin_http) << "create admin http handle failed";
  evhttp_set_cb(p_->admin_http, "/pub", PubHandler, this);
  evhttp_set_cb(p_->admin_http, "/broadcast", BroadcastHandler, this);
  evhttp_set_cb(p_->admin_http, "/stats", StatsHandler, this);
  evhttp_set_cb(p_->admin_http, "/disconnect", DisconnectHandler, this);
  evhttp_set_cb(p_->admin_http, "/sub", SubHandler, this);
  evhttp_set_cb(p_->admin_http, "/unsub", UnsubHandler, this);
  evhttp_set_cb(p_->admin_http, "/msg", MsgHandler, this);
  evhttp_set_cb(p_->admin_http, "/shard", ShardHandler, this);

  struct evhttp_bound_socket* sock = NULL;
  sock = evhttp_bind_socket_with_handle(p_->admin_http,
                                        "0.0.0.0",
                                        admin_listen_port_);
  CHECK(sock) << "bind address failed: " << strerror(errno);

  LOG(INFO) << "admin server listen on " << admin_listen_port_;

  struct evconnlistener *listener = evhttp_bound_socket_get_listener(sock);
  evconnlistener_set_error_cb(listener, AcceptErrorHandler);
}

void SessionServer::SetupEventHandler() {
	p_->sigint_event = evsignal_new(p_->evbase, SIGINT, SignalHandler, this);
  CHECK(p_->sigint_event && event_add(p_->sigint_event, NULL) == 0)
      << "set SIGINT handler failed";

	p_->sigterm_event = evsignal_new(p_->evbase, SIGTERM, SignalHandler, this);
  CHECK(p_->sigterm_event && event_add(p_->sigterm_event, NULL) == 0)
      << "set SIGTERM handler failed";

	p_->timer_event = event_new(p_->evbase, -1, EV_PERSIST, TimerHandler, this);
  struct timeval tv;
  tv.tv_sec = FLAGS_timer_interval_sec;
  tv.tv_usec = 0;
  CHECK(p_->timer_event&& event_add(p_->timer_event, &tv) == 0)
      << "set timer handler failed";
}

}  // namespace xcomet

const char* const SERVER_PID_FILE = "session_server.pid";

static void WritePidFile() {
  base::File::WriteStringToFileOrDie(std::to_string(::getpid()),
      SERVER_PID_FILE);
}

int main(int argc, char* argv[]) {
  base::AtExitManager at_exit;
  base::ParseCommandLineFlags(&argc, &argv, false);
  base::daemonize();
  if (FLAGS_flagfile.empty()) {
    LOG(WARNING) << "not using --flagfile option !";
  }
  LOG(INFO) << "command line options\n" << base::CommandlineFlagsIntoString();
  WritePidFile();
  {
    xcomet::SessionServer server;
    server.Start();
    LOG(INFO) << "main loop break";
  }
  base::File::DeleteRecursively(SERVER_PID_FILE);
}
