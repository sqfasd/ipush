#include "src/session_server.h"

#include <evhttp.h>
#include <event2/event.h>
#include <event2/listener.h>
#include "base/logging.h"
#include "base/flags.h"
#include "base/string_util.h"
#include "base/shared_ptr.h"

DEFINE_int32(client_listen_port, 9000, "");
DEFINE_int32(admin_listen_port, 9100, "");
DEFINE_int32(poll_timeout_sec, 30, "");
DEFINE_int32(timer_interval_sec, 1, "");

#define CHECK_HTTP_GET()\
  do {\
    if(evhttp_request_get_command(req) != EVHTTP_REQ_GET) {\
      evhttp_send_reply(req, 405, "Method Not Allowed", NULL);\
      return;\
    }\
  } while(0)

namespace xcomet {

SessionServer::SessionServer()
    : timeout_queue_(FLAGS_poll_timeout_sec / FLAGS_timer_interval_sec) {
}

SessionServer::~SessionServer() {
}

void SessionServer::Sub(struct evhttp_request* req) {
  CHECK_HTTP_GET();

  HttpQuery query(req);
  string uid = query.GetStr("uid", "");
  if (uid.empty()) {
    evhttp_send_reply(req, 410, "Invalid parameters", NULL);
    return;
  }

  int type = query.GetInt("type", 1);
  // string token = query.GetStr("token", "");
  // TODO check request parameters

  UserPtr user;
  UserMap::iterator iter = users_.find(uid);
  if (iter != users_.end()) {
    user = iter->second;
    user->SetType(type);
    LOG(INFO) << "user has already connected: " << uid;
    evhttp_send_reply(req, 406, "The user has already connected", NULL);
    return;
  } else {
    user.reset(new User(uid, type, req, *this));
    users_[uid] = user;
    router_.RegisterUser(uid);
  }
  timeout_queue_.PushUserBack(user.get());
}

// /pub?uid=123&content=hello
// /pub?cid=123&content=hello
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
  string uid = query.GetStr("uid", "");

  if (!uid.empty()) {
    LOG(INFO) << "pub to user: " << content;
    UserMap::iterator iter = users_.find(uid);
    if (iter != users_.end()) {
      UserPtr user = iter->second;
      LOG(INFO) << "send to user: " << user->GetUid();
      if (post_buffer->empty()) {
        user->Send(content);
      } else {
        user->Send(*(post_buffer.get()));
      }
      timeout_queue_.PushUserBack(user.get());
    } else {
      LOG(ERROR) << "pub uid not found: " << uid;
      evhttp_send_reply(req, 404, "Not Found", NULL);
      return;
    }
  } else {
    string cid = query.GetStr("cid", "");
    if (cid.empty()) {
      LOG(WARNING) << "uid and cid both empty";
      evhttp_send_reply(req, 410, "Invalid parameters", NULL);
      return;
    }
    LOG(INFO) << "pub to channel: " << content;
    ChannelMap::iterator iter = channels_.find(cid);
    if (iter != channels_.end()) {
      iter->second->Broadcast(content);
    } else {
      LOG(ERROR) << "pub cid not found: " << uid;
      evhttp_send_reply(req, 404, "Not Found", NULL);
      return;
    }
  }
  ReplyOK(req);
}

// /unsub?uid=123&resource=abc
void SessionServer::Unsub(struct evhttp_request* req) {
}

// /createroom?uid=123
// return roomid
void SessionServer::CreateRoom(struct evhttp_request* req) {
}

void SessionServer::DestroyRoom(struct evhttp_request* req) {
}

// /broadcast?content=hello
void SessionServer::Broadcast(struct evhttp_request* req) {
}

// /rsub?seq=1
void SessionServer::RSub(struct evhttp_request* req) {
  CHECK_HTTP_GET();
  router_.ResetSession(req);
}

// /join?cid=123&uid=456&resource=work
void SessionServer::Join(struct evhttp_request* req) {
}

// /leave?cid=123&uid=456&resource=work
void SessionServer::Leave(struct evhttp_request* req) {
}

void SessionServer::OnTimer() {
  VLOG(3) << "OnTimer";
  DLinkedList<User*> timeout_users = timeout_queue_.PopFront();
  DLinkedList<User*>::Iterator it = timeout_users.GetIterator();
  while (User* user = it.Next()) {
    if (user->GetType() == User::COMET_TYPE_POLLING) {
      user->Close();
    } else {
      user->SendHeartbeat();
      timeout_queue_.PushUserBack(user);
    }
  }
  timeout_queue_.IncHead();
  if (router_.IncCounter() ==
      FLAGS_poll_timeout_sec / FLAGS_timer_interval_sec) {
    router_.SendHeartbeat();
    router_.SetCounter(0);
  }
}

void SessionServer::RemoveUser(User* user) {
  const string& uid = user->GetUid();
  LOG(INFO) << "RemoveUser: " << uid;
  timeout_queue_.RemoveUser(user);
  router_.UnregisterUser(uid);
  users_.erase(uid);
}

void SessionServer::ReplyOK(struct evhttp_request* req) {
  evhttp_add_header(req->output_headers, "Content-Type", "text/javascript; charset=utf-8");
  struct evbuffer *buf = evbuffer_new();
  evbuffer_add_printf(buf, "{\"type\":\"ok\"}\n");
  evhttp_send_reply(req, 200, "OK", buf);
  evbuffer_free(buf);
}

}  // namespace xcomet

using xcomet::SessionServer;

static void SubHandler(struct evhttp_request* req, void* arg) {
  SessionServer::Instance().Sub(req);
}

static void PubHandler(struct evhttp_request* req, void* arg) {
  SessionServer::Instance().Pub(req);
}

static void UnsubHandler(struct evhttp_request* req, void* arg) {
  SessionServer::Instance().Unsub(req);
}

static void BroadcastHandler(struct evhttp_request* req, void* arg) {
  SessionServer::Instance().Broadcast(req);
}

static void RSubHandler(struct evhttp_request* req, void* arg) {
  SessionServer::Instance().RSub(req);
}

static void JoinHandler(struct evhttp_request* req, void* arg) {
  SessionServer::Instance().Join(req);
}

static void LeaveHandler(struct evhttp_request* req, void* arg) {
  SessionServer::Instance().Leave(req);
}

static void AcceptErrorHandler(struct evconnlistener* listener, void* ptr) {
}

static void SignalHandler(evutil_socket_t sig, short events, void* user_data) {
  struct event_base* evbase = (struct event_base*)user_data;
	event_base_loopbreak(evbase);
}

static void TimerHandler(evutil_socket_t sig, short events, void *user_data) {
  SessionServer::Instance().OnTimer();
}

void ParseIpPort(const string& address, string& ip, int& port) {
  size_t pos = address.find(':');
  if (pos == string::npos) {
    ip = "";
    port = 0;
    return;
  }
  ip = address.substr(0, pos);
  port = StringToInt(address.substr(pos+1, address.length()));
}

void SetupClientHandler(struct evhttp* http, struct event_base* evbase) {
  evhttp_set_cb(http, "/sub", SubHandler, evbase);
  evhttp_set_cb(http, "/unsub", UnsubHandler, evbase);
  evhttp_set_cb(http, "/join", JoinHandler, evbase);
  evhttp_set_cb(http, "/leave", LeaveHandler, evbase);

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

	event_base_dispatch(evbase);

  LOG(INFO) << "event loop break";

	event_free(timer_event);
	event_free(sigterm_event);
	event_free(sigint_event);
	evhttp_free(client_http);
	evhttp_free(admin_http);
	event_base_free(evbase);
}
