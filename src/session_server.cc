#include "src/session_server.h"

#include "deps/jsoncpp/include/json/json.h"
#include "deps/base/logging.h"
#include "deps/base/flags.h"
#include "deps/base/shared_ptr.h"
#include "deps/base/daemonizer.h"
#include "src/loop_executor.h"

DEFINE_int32(client_listen_port, 9000, "");
DEFINE_int32(admin_listen_port, 9100, "");
DEFINE_int32(poll_timeout_sec, 300, "");
DEFINE_int32(timer_interval_sec, 1, "");
DEFINE_bool(is_server_heartbeat, false, "");

#define CHECK_HTTP_GET()\
  do {\
    if(evhttp_request_get_command(req) != EVHTTP_REQ_GET) {\
      ReplyError(req, HTTP_BADMETHOD);\
      return;\
    }\
  } while(0)

namespace xcomet {

SessionServer::SessionServer()
    : timeout_counter_(FLAGS_poll_timeout_sec / FLAGS_timer_interval_sec),
      timeout_queue_(timeout_counter_),
      stats_(FLAGS_timer_interval_sec) {
}

SessionServer::~SessionServer() {
}

// /connect?uid=123&token=ABCDE&type=1|2
void SessionServer::Connect(struct evhttp_request* req) {
  CHECK_HTTP_GET();
  HttpQuery query(req);
  string uid = query.GetStr("uid", "");
  if (uid.empty()) {
    ReplyError(req, HTTP_BADREQUEST, "invalid uid");
    return;
  }

  int type = query.GetInt("type", 1);
  string token = query.GetStr("token", "");
  // TODO (qingfeng) authenticate token

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
}

// /pub?to=123&content=hello&seq=1&from=unknow
// @DEPRECATED
void SessionServer::Pub(struct evhttp_request* req) {
  struct evbuffer* input_buffer = evhttp_request_get_input_buffer(req);
  int len = evbuffer_get_length(input_buffer);
  VLOG(3) << "Pub receive data length: " << len;
  shared_ptr<string> post_buffer(new string());
  post_buffer->reserve(len);
  post_buffer->append((char*)evbuffer_pullup(input_buffer, -1), len);

  HttpQuery query(req);
  string content = query.GetStr("content", "");
  string to = query.GetStr("to", "");
  string from = query.GetStr("from", "unknow");

  if (!to.empty()) {
    UserMap::iterator iter = users_.find(to);
    if (iter != users_.end()) {
      UserPtr user = iter->second;
      LOG(INFO) << "send to user: " << user->GetId();
      if (post_buffer->empty()) {
        user->Send(from, "pub", content);
      } else {
        user->Send(from, "pub", *(post_buffer.get()));
      }
    } else {
      ReplyError(req, HTTP_INTERNAL, "target not found: " + to);
      return;
    }
  } else {
    ReplyError(req, HTTP_BADREQUEST, "target user id is empty");
    return;
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

  UserMap::iterator uit = users_.find(uid);
  if (uit == users_.end()) {
    ReplyError(req, HTTP_INTERNAL, "user not found: " + uid);
  } else {
    uit->second->Close();
    ReplyOK(req);
  }
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

bool SessionServer::IsHeartbeatMessage(const string& message) {
  return message.find("\"type\"") != string::npos &&
         message.find("\"noop\"") != string::npos;
}

void SessionServer::OnUserMessage(const string& from_uid, StringPtr message) {
  stats_.OnReceive(*message);
  VLOG(2) << from_uid << ": " << *message;
  if (!IsHeartbeatMessage(*message)) {
    // router_.Redirect(message);
  }

  UserMap::iterator uit = users_.find(from_uid);
  if (uit == users_.end()) {
    LOG(WARNING) << "user not found: " << from_uid;
  } else {
    timeout_queue_.PushUserBack(uit->second.get());
  }
}

void SessionServer::OnRouterMessage(shared_ptr<string> message) {
  VLOG(3) << "OnRouterMessage: " << *message;
  if (IsHeartbeatMessage(*message)) {
    VLOG(5) << "is router heartbeat, ignore";
    return;
  }
  try {
    Json::Reader reader;
    Json::Value json;
    int ret = reader.parse(*message, json);
    CHECK(ret) << "json format error";
    if (!json.isMember("to")) {
      return;
    }
    const string& uid = json["to"].asString();
    UserMap::iterator uit = users_.find(uid);
    if (uit == users_.end()) {
      LOG(WARNING) << "user not found: " << uid;
    } else {
      stats_.OnSend(*message);
      uit->second->SendPacket(*message);
    }
  } catch (std::exception& e) {
    LOG(ERROR) << "OnRouterMessage json exception: " << e.what()
               << ", msg = " << *message;
  } catch (...) {
    LOG(ERROR) << "OnRouterMessage unknow exception";
  }
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

void SessionServer::OnStart() {
  stats_.OnServerStart();
}

}  // namespace xcomet

using xcomet::SessionServer;

static void ConnectHandler(struct evhttp_request* req, void* arg) {
  LOG(INFO) << "request: " << evhttp_request_get_uri(req);
  SessionServer::Instance().Connect(req);
}

static void DisconnectHandler(struct evhttp_request* req, void* arg) {
  LOG(INFO) << "request: " << evhttp_request_get_uri(req);
  SessionServer::Instance().Disconnect(req);
}

static void PubHandler(struct evhttp_request* req, void* arg) {
  LOG(INFO) << "request: " << evhttp_request_get_uri(req);
  SessionServer::Instance().Pub(req);
}

static void BroadcastHandler(struct evhttp_request* req, void* arg) {
  LOG(INFO) << "request: " << evhttp_request_get_uri(req);
  SessionServer::Instance().Broadcast(req);
}

static void StatsHandler(struct evhttp_request* req, void* arg) {
  LOG(INFO) << "request: " << evhttp_request_get_uri(req);
  SessionServer::Instance().Stats(req);
}

static void AcceptErrorHandler(struct evconnlistener* listener, void* ptr) {
  LOG(ERROR) << "AcceptErrorHandler";
}

static void SignalHandler(evutil_socket_t sig, short events, void* user_data) {
  LOG(INFO) << "SignalHandler: " << sig << ", " << events;
  struct event_base* evbase = (struct event_base*)user_data;
	event_base_loopbreak(evbase);
}

static void TimerHandler(evutil_socket_t sig, short events, void *user_data) {
  SessionServer::Instance().OnTimer();
}

void SetupClientHandler(struct evhttp* http, struct event_base* evbase) {
  evhttp_set_cb(http, "/connect", ConnectHandler, evbase);
  evhttp_set_cb(http, "/disconnect", DisconnectHandler, evbase);

  struct evhttp_bound_socket* sock = NULL;
  sock = evhttp_bind_socket_with_handle(http,
                                        "0.0.0.0",
                                        FLAGS_client_listen_port);
  CHECK(sock) << "bind address failed: " << strerror(errno);

  LOG(INFO) << "clinet server listen on " << FLAGS_client_listen_port;

  struct evconnlistener *listener = evhttp_bound_socket_get_listener(sock);
  evconnlistener_set_error_cb(listener, AcceptErrorHandler);
}

void SetupAdminHandler(struct evhttp* http, struct event_base* evbase) {
  evhttp_set_cb(http, "/pub", PubHandler, evbase);
  evhttp_set_cb(http, "/broadcast", BroadcastHandler, evbase);
  evhttp_set_cb(http, "/stats", StatsHandler, evbase);

  struct evhttp_bound_socket* sock = NULL;
  sock = evhttp_bind_socket_with_handle(http,
                                        "0.0.0.0",
                                        FLAGS_admin_listen_port);
  CHECK(sock) << "bind address failed: " << strerror(errno);

  LOG(INFO) << "admin server listen on " << FLAGS_admin_listen_port;

  struct evconnlistener *listener = evhttp_bound_socket_get_listener(sock);
  evconnlistener_set_error_cb(listener, AcceptErrorHandler);
}

int main(int argc, char* argv[]) {
  base::AtExitManager at_exit;
  base::ParseCommandLineFlags(&argc, &argv, false);
  base::daemonize();
  if (FLAGS_flagfile.empty()) {
    LOG(WARNING) << "not using --flagfile option !";
  }
  LOG(INFO) << "command line options\n" << base::CommandlineFlagsIntoString();

  struct event_base* evbase = NULL;
  evbase = event_base_new();
  CHECK(evbase) << "create evbase failed";

  struct evhttp* client_http = NULL;
  client_http = evhttp_new(evbase);
  CHECK(client_http) << "create client http handle failed";
  SetupClientHandler(client_http, evbase);

  struct evhttp* admin_http = NULL;
  admin_http = evhttp_new(evbase);
  CHECK(admin_http) << "create admin http handle failed";
  SetupAdminHandler(admin_http, evbase);

  struct event* sigint_event = NULL;
	sigint_event = evsignal_new(evbase, SIGINT, SignalHandler, evbase);
  CHECK(sigint_event && event_add(sigint_event, NULL) == 0)
      << "set SIGINT handler failed";

  struct event* sigterm_event = NULL;
	sigterm_event = evsignal_new(evbase, SIGTERM, SignalHandler, evbase);
  CHECK(sigterm_event && event_add(sigterm_event, NULL) == 0)
      << "set SIGTERM handler failed";

  struct event* timer_event = NULL;
	timer_event = event_new(evbase, -1, EV_PERSIST, TimerHandler, evbase);
	{
		struct timeval tv;
		tv.tv_sec = FLAGS_timer_interval_sec;
		tv.tv_usec = 0;
    CHECK(sigterm_event && event_add(timer_event, &tv) == 0)
        << "set timer handler failed";
	}

  LoopExecutor::Init(evbase);
  SessionServer::Instance().OnStart();

	event_base_dispatch(evbase);

  LOG(INFO) << "main loop break";

	event_free(timer_event);
	event_free(sigterm_event);
	event_free(sigint_event);
	evhttp_free(client_http);
	evhttp_free(admin_http);
	event_base_free(evbase);
}
