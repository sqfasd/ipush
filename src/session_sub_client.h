#ifndef SESSION_SUB_CLIENT_H
#define SESSION_SUB_CLIENT_H

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/http.h>
#include "include_std.h"

namespace xcomet {

class SessionSubClient {
 public:
  SessionSubClient(
      class RouterServer* parent,
      struct event_base *evbase,
      size_t client_id,
      const string& sub_host, 
      int sub_port, 
      const string& sub_uri
      );
  ~SessionSubClient();
 public:
  void Reconnect();
  void MakeReSubEvent();
  void MakeSubEvent();
 private:
  void InitConn();
  void CloseConn();
 private: 
  static void ReSubCB(int sock, short which, void * ctx);
  static void ConnCloseCB(struct evhttp_connection * conn, void *ctx);
 private:
  static void SubDoneCB(struct evhttp_request *req, void * ctx);// this class
  static void SubChunkCB(struct evhttp_request *req, void * ctx);// this class 
  static void SubCompleteCB(struct evhttp_request *req, void * ctx);
 private:
  class RouterServer* parent_;
  struct event_base *evbase_;
 private:
  size_t client_id_;
  string sub_host_;
  int sub_port_;
  string sub_uri_;

 private:
  struct evhttp_connection* evhttpcon_;
};

} // namespace xcomet


#endif
