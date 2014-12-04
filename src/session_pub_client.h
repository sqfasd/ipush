#ifndef SESSION_PUB_CLIENT_H
#define SESSION_PUB_CLIENT_H

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/http.h>
#include <evhttp.h>
//#include <event2/evhttp.h>
#include "include_std.h"

namespace xcomet {

class SessionPubClient {
 public:
  SessionPubClient(
      struct event_base *evbase,
      size_t client_id,
      const string& pub_host, 
      int pub_port
      );
  ~SessionPubClient();
 public:
  void MakePubEvent(const char * uid, const char* content, size_t len);
 private:
  void InitConn();
  void CloseConn();
 private: // callbacks
  static void ConnCloseCB(struct evhttp_connection * conn, void *ctx);
 private:
  //static void ReqErrorCB(enum evhttp_request_error err, void * ctx);
 private:
  static void PubDoneCB(struct evhttp_request* req, void * ctx);
  static void PubCompleteCB(struct evhttp_request* req, void *ctx);
 private:
  struct event_base *evbase_;
 private:
  size_t client_id_;
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
