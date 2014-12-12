#ifndef ROUTER_SERVER_H
#define ROUTER_SERVER_H

#include "deps/base/shared_ptr.h"

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

#include "deps/base/scoped_ptr.h"
#include "deps/base/callback.h"
#include "src/storage.h"
#include "src/http_client.h"
#include "utils.h"

using namespace std;

namespace xcomet {

typedef string UserID;
typedef int Sid; 
const Sid INVALID_SID = -1;

using base::shared_ptr;

class RouterServer {
 public:
  RouterServer();
  ~RouterServer();

  void Start();
 private:

  static void OnSubMsg(const HttpClient* client, const string& msg, void *ctx);
  static void OnSubRequestDone(const HttpClient* client, const string& resp, void*ctx);

  void OpenSubClients();
  void OpenSubClient(Sid sid);
  void CloseSubClient(Sid sid);

  void InitStorage();
  void InitAdminHttp();
  
  Sid  FindSidByUid(const UserID& uid) const;
  void InsertUid(const UserID& uid, Sid sid);
  void EraseUid(const UserID& uid) ;

  void OnPushMsgDone(bool ok);

  void OnGetMsg(UserID uid, int64_t start, MessageIteratorPtr mit);
  void OnGetMsgToReply(UserID uid, struct evhttp_request * req, MessageIteratorPtr mit);

  void GetMsg(const UserID& uid, int64_t start, boost::function<void (MessageIteratorPtr)> cb);

  void SendUserMsg(const UserID& uid, const string& msg);

  static void OnAdminPub(struct evhttp_request* req, void *ctx);
  static void OnAdminBroadcast(struct evhttp_request* req, void *ctx);
  static void OnAdminCheckPresence(struct evhttp_request* req, void * ctx);
  static void OnAdminCheckOffMsg(struct evhttp_request* req, void *ctx);
  static void OnAcceptError(struct evconnlistener * listener , void *ctx);

  static void SendReply(struct evhttp_request* req,  const char* content, size_t len);

  static void ReplyError(struct evhttp_request* req);
  static void ReplyOK(struct evhttp_request* req);

  map<UserID, Sid> u2sMap_;
  vector<shared_ptr<HttpClient> > sub_clients_;

  struct event_base *evbase_;
  struct evhttp* admin_http_;

  scoped_ptr<Storage> storage_;
};

} // namespace xcomet


#endif
