#include "src/xcomet_server.h"

#include <signal.h>
#include <evhttp.h>
#include <event2/event.h>
#include <event2/listener.h>
#include "base/logging.h"
#include "base/flags.h"
#include "base/string_util.h"

DEFINE_int32(client_listen_port, 9000, "");
DEFINE_int32(admin_listen_port, 9100, "");

using xcomet::XCometServer;

static void SubHandler(struct evhttp_request* req, void* arg) {
  XCometServer::Instance().Sub(req);
}

static void PubHandler(struct evhttp_request* req, void* arg) {
  XCometServer::Instance().Pub(req);
}

static void UnsubHandler(struct evhttp_request* req, void* arg) {
  XCometServer::Instance().Unsub(req);
}

static void BroadcastHandler(struct evhttp_request* req, void* arg) {
  XCometServer::Instance().Broadcast(req);
}

static void JoinHandler(struct evhttp_request* req, void* arg) {
  XCometServer::Instance().Join(req);
}

static void LeaveHandler(struct evhttp_request* req, void* arg) {
  XCometServer::Instance().Leave(req);
}

static void AcceptErrorHandler(struct evconnlistener* listener, void* ptr) {
}

static void SignalHandler(evutil_socket_t sig, short events, void* user_data) {
  struct event_base* evbase = (struct event_base*)user_data;
	event_base_loopbreak(evbase);
}

static void TimerHandler(evutil_socket_t sig, short events, void *user_data) {
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
		tv.tv_sec = 1;
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
