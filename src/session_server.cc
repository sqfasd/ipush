#include "src/session_server.h"

#include <evhttp.h>
#include <event2/event.h>
#include <event2/listener.h>
#include "deps/jsoncpp/include/json/json.h"
#include "deps/base/logging.h"
#include "deps/base/flags.h"
#include "base/shared_ptr.h"
#include "utils.h"

DEFINE_int32(client_listen_port, 9000, "");
DEFINE_int32(admin_listen_port, 9100, "");
DEFINE_int32(poll_timeout_sec, 30, "");
DEFINE_int32(timer_interval_sec, 1, "");
DEFINE_bool(is_server_heartbeat, false, "");

#define CHECK_HTTP_GET()\
  do {\
    if(evhttp_request_get_command(req) != EVHTTP_REQ_GET) {\
      evhttp_send_reply(req, 405, "Method Not Allowed", NULL);\
      return;\
    }\
  } while(0)

namespace xcomet {

SessionServer::SessionServer()
    : router_(*this),
      timeout_queue_(FLAGS_poll_timeout_sec / FLAGS_timer_interval_sec),
      stats_(FLAGS_timer_interval_sec) {
  register_all_ = false;
}

SessionServer::~SessionServer() {
}

// /connect?uid=123&token=ABCDE&type=1|2
void SessionServer::Connect(struct evhttp_request* req) {
  CHECK_HTTP_GET();

  HttpQuery query(req);
  string uid = query.GetStr("uid", "");
  if (uid.empty()) {
    evhttp_send_reply(req, 410, "Invalid parameters", NULL);
    return;
  }

  int type = query.GetInt("type", 1);
  string token = query.GetStr("token", "");
  // TODO check request parameters

  UserPtr user;
  UserMap::iterator iter = users_.find(uid);
  if (iter != users_.end()) {
    user = iter->second;
    user->SetType(type);
    // TODO(qingfeng) kick off the old connection if reconnected
    LOG(INFO) << "user has already connected: " << uid;
    evhttp_send_reply(req, 406, "The user has already connected", NULL);
    return;
  } else {
    user.reset(new User(uid, type, req, *this));
    users_[uid] = user;
    router_.LoginUser(uid);
  }
  timeout_queue_.PushUserBack(user.get());
}

// /pub?to=123&content=hello&seq=1&from=unknow
void SessionServer::Pub(struct evhttp_request* req) {
  // TODO process post
  // CHECK_HTTP_GET();
  struct evbuffer* input_buffer = evhttp_request_get_input_buffer(req);
  int len = evbuffer_get_length(input_buffer);
  VLOG(3) << "Pub receive data length: " << len;
  base::shared_ptr<string> post_buffer(new string());
  post_buffer->reserve(len);
  post_buffer->append((char*)evbuffer_pullup(input_buffer, -1), len);

  HttpQuery query(req);
  string content = query.GetStr("content", "");
  string to = query.GetStr("to", "");
  string from = query.GetStr("from", "unknow");

  if (!to.empty()) {
    LOG(INFO) << "pub to user: " << content;
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
      LOG(ERROR) << "pub uid not found: " << to;
      evhttp_send_reply(req, 404, "Not Found", NULL);
      return;
    }
  } else {
    LOG(WARNING) << "invalid parameter";
    evhttp_send_reply(req, 410, "Invalid parameters", NULL);
    return;
  }
  ReplyOK(req);
}

void SessionServer::Disconnect(struct evhttp_request* req) {
  CHECK_HTTP_GET();

  HttpQuery query(req);
  string uid = query.GetStr("uid", "");
  if (uid.empty()) {
    evhttp_send_reply(req, 410, "Invalid parameters", NULL);
    return;
  }

  UserMap::iterator uit = users_.find(uid);
  if (uit == users_.end()) {
    LOG(WARNING) << "user not found: " << uid;
  } else {
    uit->second->Close();
  }
  ReplyOK(req);
}

// /broadcast?content=hello
void SessionServer::Broadcast(struct evhttp_request* req) {
}

// /rsub?seq=1
void SessionServer::RSub(struct evhttp_request* req) {
  CHECK_HTTP_GET();
  // TODO (qingfeng) check request parameters
  router_.ResetSession(req);
  LOG(INFO) << "router connected, there are " << users_.size()
            << " users to register";
  // TODO(qingfeng) use copy-on-write method to login all users
  register_all_ = true;
}

void SessionServer::OnTimer() {
  VLOG(5) << "OnTimer";
  stats_.OnTimer();
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

  if (router_.IncCounter() ==
      FLAGS_poll_timeout_sec / FLAGS_timer_interval_sec) {
    router_.SendHeartbeat();
    router_.SetCounter(0);
  }
  if (register_all_) {
    UserMap::iterator it;
    for (it = users_.begin(); it != users_.end(); ++it) {
      router_.LoginUser(it->first);
    }
    register_all_ = false;
  }
}

bool SessionServer::IsHeartbeatMessage(StringPtr message) {
  return message.get() &&
         message->size() == 16 &&
         message->find("noop") != string::npos;
}

void SessionServer::OnUserMessage(const string& from_uid, StringPtr message) {
  stats_.OnUserMessage(message);
  // TODO(qingfeng) if target in current session, send it before redirect
  LOG(INFO) << from_uid << ": " << *message;
  if (!IsHeartbeatMessage(message)) {
    router_.Redirect(message);
  }

  UserMap::iterator uit = users_.find(from_uid);
  if (uit == users_.end()) {
    LOG(WARNING) << "user not found: " << from_uid;
  } else {
    timeout_queue_.PushUserBack(uit->second.get());
  }
}

void SessionServer::OnRouterMessage(base::shared_ptr<string> message) {
  VLOG(5) << "OnRouterMessage: " << *message;
  stats_.OnPubMessage(message);
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
      uit->second->SendPacket(*message);
    }
  } catch (std::exception& e) {
    CHECK(false) << "OnRouterMessage json exception: " << e.what();
  } catch (...) {
    CHECK(false) << "OnRouterMessage unknow exception";
  }
}

void SessionServer::RemoveUser(User* user) {
  const string& uid = user->GetId();
  LOG(INFO) << "RemoveUser: " << uid;
  timeout_queue_.RemoveUser(user);
  router_.LogoutUser(uid);
  users_.erase(uid);
}

void SessionServer::ReplyOK(struct evhttp_request* req, const string& resp) {
  evhttp_add_header(req->output_headers,
                    "Content-Type",
                    "text/json; charset=utf-8");
  struct evbuffer * output_buffer = evhttp_request_get_output_buffer(req);
  if (resp.empty()) {
    evbuffer_add_printf(output_buffer, "{\"result\":\"ok\"}\n");
  } else {
    evbuffer_add(output_buffer, resp.c_str(), resp.size());
  }
  evhttp_send_reply(req, 200, "OK", output_buffer);
}

void SessionServer::Stats(struct evhttp_request* req) {
  // TODO(qingfeng) check pretty json format parameter
  Json::Value response;
  Json::Value& result = response["result"];
  result["user_count"] = (Json::UInt)users_.size();
  stats_.GetReport(response);
  ReplyOK(req, response.toStyledString());
}

void SessionServer::OnStart() {
  stats_.OnServerStart();
}

}  // namespace xcomet

using xcomet::SessionServer;

static void ConnectHandler(struct evhttp_request* req, void* arg) {
  SessionServer::Instance().Connect(req);
}

static void DisconnectHandler(struct evhttp_request* req, void* arg) {
  SessionServer::Instance().Disconnect(req);
}

static void PubHandler(struct evhttp_request* req, void* arg) {
  SessionServer::Instance().Pub(req);
}

static void BroadcastHandler(struct evhttp_request* req, void* arg) {
  SessionServer::Instance().Broadcast(req);
}

static void RSubHandler(struct evhttp_request* req, void* arg) {
  SessionServer::Instance().RSub(req);
}

static void StatsHandler(struct evhttp_request* req, void* arg) {
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
  evhttp_set_cb(http, "/rsub", RSubHandler, evbase);
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
