#include "deps/jsoncpp/include/json/json.h"
#include "deps/base/flags.h"
#include "deps/base/logging.h"
#include "router_server.h"
#include "deps/base/time.h"
#include "deps/base/string_util.h"

DEFINE_string(sserver_sub_ips, "127.0.0.1|127.0.0.1", "");
DEFINE_string(sserver_sub_ports, "8100|8200", "");
DEFINE_string(sserver_sub_uri, "/stream?cname=12", "");
DEFINE_bool(libevent_debug_log, false, "for debug logging");

namespace xcomet {

RouterServer::RouterServer() { 
  evbase_ = event_base_new();
  CHECK(evbase_);
}

RouterServer::~RouterServer() {
  event_base_free(evbase_);
}

void RouterServer::Start() {
  InitSubClients();
  event_base_dispatch(evbase_);
}

void RouterServer::InitSubClients() {
  VLOG(5) << FLAGS_sserver_sub_ips ;
  VLOG(5) << FLAGS_sserver_sub_ports ;
  vector<string> ips;
  vector<string> ports;
  SplitString(FLAGS_sserver_sub_ips, '|', &ips);
  SplitString(FLAGS_sserver_sub_ports, '|', &ports);
  CHECK(subclients_.empty());
  CHECK(ips.size());
  CHECK(ips.size() == ports.size());
  for (size_t i = 0; i < ips.size(); i++) {
    LOG(INFO) << "new SessionClient " << ips[i] << "," << ports[i];
    subclients_.push_back(
                shared_ptr<SessionClient>(
                    new SessionClient(
                        evbase_, 
                        ClientErrorCB,
                        ips[i], 
                        atoi(ports[i].c_str()),
                        FLAGS_sserver_sub_uri
                        )
                    )
                );
  }
  // init request events
  for(size_t i = 0; i < subclients_.size(); i++) {
    subclients_[i]->MakeRequestEvent();
  }
}

void RouterServer::ClientErrorCB(int sock, short which, void *arg) {
  SessionClient * client = static_cast<SessionClient*>(arg);
  CHECK(client != NULL);
  VLOG(5) << "ClientErrorCB";
  //TODO
}

#if 0
//TODO to close connection
void RouterServer::MakePubReq(const char* host, const int port, const char* uri) {
  //struct bufferevent * bev = bufferevent_socket_new(evbase_, -1, BEV_OPT_CLOSE_ON_FREE);
  //CHECK(bev);
  //const char * host = "slave1.domain.com";
  //const int port = 9101;
  struct evhttp_connection * evhttpcon = evhttp_connection_base_bufferevent_new(evbase_, NULL, NULL, host, port);
  CHECK(evhttpcon);
  struct evhttp_request* req = evhttp_request_new(PubReqDoneCB, NULL);
  int r = evhttp_make_request(evhttpcon, req, EVHTTP_REQ_GET, uri);
  CHECK(r == 0);
  VLOG(5) << "MakePubReq finished.";
}
#endif

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

