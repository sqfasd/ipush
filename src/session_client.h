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
      const string& sub_host, 
      int sub_port, 
      const string& sub_uri,
      const string& pub_host,
      int pub_port
      );
  ~SessionClient();
 public:
  void MakeSubEvent();
  void MakePubEvent(const char* pub_uri);
 private:
  void InitConn();
  void CloseConn();
 private: // callbacks
  static void ConnCloseCB(struct evhttp_connection * conn, void *ctx);
 private:
  static void SubDoneCB(struct evhttp_request *req, void * ctx);// this class
  static void SubChunkCB(struct evhttp_request *req, void * ctx);// this class 
  static void SubCompleteCB(struct evhttp_request *req, void * ctx);
 private:
  static void ReqErrorCB(enum evhttp_request_error err, void * ctx);
 private:
  static void PubDoneCB(struct evhttp_request* req, void * ctx);
  static void PubCompleteCB(struct evhttp_request* req, void *ctx);
 private:
  class RouterServer* parent_;
  struct event_base *evbase_;
 private:
  size_t client_id_;
  string sub_host_;
  int sub_port_;
  string sub_uri_;

 private:
  string pub_host_;
  int pub_port_;

 private:
  struct evhttp_connection* evhttpcon_;
  //struct event* everr_;
 private:
  //void InitErrorEvent(event_callback_fn fn);
  //void ActiveErrorEvent();
};

} // namespace xcomet


#endif
