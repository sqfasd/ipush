#include "deps/jsoncpp/include/json/json.h"
#include "deps/base/flags.h"
#include "deps/base/logging.h"
#include "router_server.h"
#include "deps/base/time.h"
#include "deps/base/string_util.h"

DEFINE_string(sserver_sub_ips, "127.0.0.1|127.0.0.1", "");
DEFINE_string(sserver_sub_ports, "8100|8200", "");
DEFINE_string(sserver_sub_uri, "/stream?cname=12", "");
DEFINE_int32(retry_interval, 5, "");
DEFINE_bool(libevent_debug_log, false, "for debug logging");

const struct timeval RETRY_TV = {FLAGS_retry_interval, 0};

namespace xcomet {

RouterServer::RouterServer() { 
  evbase_ = event_base_new();
  CHECK(evbase_);
}

RouterServer::~RouterServer() {
  event_base_free(evbase_);
}

void RouterServer::Start() {
  InitClientIpPorts(FLAGS_sserver_sub_ips, FLAGS_sserver_sub_ports);
  InitSubClients();
  event_base_dispatch(evbase_);
}

void RouterServer::ResetSubClient(size_t id) {
  VLOG(5) << "ResetSubClient";
  CHECK(subclients_.size());
  CHECK(clientipports_.size());
  CHECK(id < clientipports_.size()) ;
  subclients_[id].reset(
                new SessionClient(
                    this,
                    evbase_, 
                    id,
                    //ClientErrorCB,
                    clientipports_[id].first, 
                    clientipports_[id].second,
                    FLAGS_sserver_sub_uri
                    )
              );
  subclients_[id]->MakeRequestEvent();
}

void RouterServer::InitClientIpPorts(const string& ipsstr, const string& portsstr) {
  VLOG(5) << FLAGS_sserver_sub_ips ;
  VLOG(5) << FLAGS_sserver_sub_ports ;
  vector<string> ips;
  vector<string> ports;
  SplitString(ipsstr, '|', &ips);
  SplitString(portsstr, '|', &ports);
  CHECK(ips.size());
  CHECK(ips.size() == ports.size());
  clientipports_.resize(ips.size());
  LOG(INFO) << "clientipports_ size : " << clientipports_.size();
  for(size_t i = 0; i < clientipports_.size(); i++) {
    clientipports_[i].first = ips[i];
    clientipports_[i].second = atoi(ports[i].c_str());
  }
}

void RouterServer::InitSubClients() {
  CHECK(subclients_.empty());
  for (size_t i = 0; i < clientipports_.size(); i++) {
    LOG(INFO) << "new SessionClient " << clientipports_[i].first << "," << clientipports_[i].second;
    subclients_.push_back(
                shared_ptr<SessionClient>(
                    new SessionClient(
                        this,
                        evbase_, 
                        i,
                        //ClientErrorCB,
                        clientipports_[i].first, 
                        clientipports_[i].second,
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

