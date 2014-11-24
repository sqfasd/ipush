#ifndef ROUTER_SERVER_H
#define ROUTER_SERVER_H

#include "deps/base/flags.h"
#include "deps/base/logging.h"
#include <netinet/in.h>
/* For socket functions */
#include <sys/socket.h>
/* For fcntl */
#include <fcntl.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/http.h>

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <map>

#include <iostream>

using namespace std;

namespace xcomet {

class RouterServer {
  public:
    RouterServer();
    ~RouterServer();
  public:
    void Start();
  private: // callbacks
    //static void AcceptCB(evutil_socket_t listener, short event, void *arg);
    static void SubReqDoneCB(struct evhttp_request *req, void * ctx);
    static void SubReqChunkCB(struct evhttp_request *req, void * arg);
    static void ReqErrorCB(enum evhttp_request_error err, void * ctx);
  private:
    void Init();
  private:
    //map<>
  private: // socket and libevent
    //evutil_socket_t listener_;
    struct evhttp_connection* evhttpcon_;
    struct bufferevent* bev_;
    struct event_base *evbase_;
    //struct event *listener_event_;
};

} // namespace xcomet


#endif
