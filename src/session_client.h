#ifndef SESSION_CLIENT_H
#define SESSION_CLIENT_H

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/http.h>
#include "include_std.h"

namespace xcomet {

class SessionClient {
 public:
  SessionClient(
      struct event_base *evbase,
      const string& host, 
      int port, 
      const string& uri);
  ~SessionClient();
 public:
  void MakeRequestEvent();
 private:
  void InitConn();
  void CloseConn();
 private: // callbacks
  static void ConnCloseCB(struct evhttp_connection * conn, void *ctx);
  static void SubReqDoneCB(struct evhttp_request *req, void * ctx);// this class
  static void SubReqChunkCB(struct evhttp_request *req, void * ctx);// this class 
  static void ReqErrorCB(enum evhttp_request_error err, void * ctx);
  static void PubReqDoneCB(struct evhttp_request* req, void * ctx);
 private:
  string host_;
  int port_;
  string uri_;
 private:
  struct event_base *evbase_;
  struct evhttp_connection* evhttpcon_;
};

} // namespace xcomet


#endif
