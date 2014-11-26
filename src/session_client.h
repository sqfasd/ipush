#ifndef SESSION_CLIENT_H
#define SESSION_CLIENT_H

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/http.h>
#include "include_std.h"

namespace xcomet {

struct CliErrInfo {
  size_t id;
  string error;
  class RouterServer* router;
  CliErrInfo(size_t id_, const string& err, class RouterServer* parent) : id(id_), error(err), router(parent) {
  }
};

class SessionClient {
 public:
  SessionClient(
      class RouterServer* parent,
      struct event_base *evbase,
      size_t client_id,
      //event_callback_fn error_cb,
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
  class RouterServer* parent_;
  struct event_base *evbase_;
 private:
  size_t client_id_;
  string host_;
  int port_;
  string uri_;
 private:
  struct evhttp_connection* evhttpcon_;
  //struct event* everr_;
 private:
  //void InitErrorEvent(event_callback_fn fn);
  //void ActiveErrorEvent();
};

} // namespace xcomet


#endif
