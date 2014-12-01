#ifndef ROUTER_SERVER_H
#define ROUTER_SERVER_H

#include "session_sub_client.h"
#include "session_pub_client.h"
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

using namespace std;

namespace xcomet {

typedef string UserID;
typedef int SessionServerID; 
const SessionServerID INVALID_SID = -1;

using base::shared_ptr;

class RouterServer {
 public:
  RouterServer();
  ~RouterServer();
 public:
  void Start();
  void MakeCliErrEvent(CliErrInfo* clierr);
  void MakePubEvent(const char* uid, const char* data, size_t len);
  void ChunkedMsgHandler(size_t sid, const char* buffer, size_t len);
 private:
  void ResetSubClient(size_t id);
  void InitSubCliAddrs(const string& ips, const string& ports);
  void InitSubClients();
 private:
  void InitPubCliAddrs(const string& ips, const string& ports);
  void InitPubClients();
 private:
  void InitStorage(const string& path);
 private:
  map<UserID, SessionServerID> u2sMap_;
 private:
  SessionServerID FindServerIdByUid(const UserID& uid) const;
  void InsertUid(const UserID& uid, SessionServerID sid);
  void EraseUid(const UserID& uid) ;
 private:
  vector<pair<string, size_t> > subcliaddrs_;
  vector<pair<string, size_t> > pubcliaddrs_;
  vector<shared_ptr<SessionSubClient> > session_sub_clients_;
  vector<shared_ptr<SessionPubClient> > session_pub_clients_;
 private: //callbacks
  static void ClientErrorCB(int sock, short which, void *arg);
 private:
  void PushOfflineMsgDoneCB(bool ok);
  void PopOfflineMsgDoneCB(UserID uid, MessageIteratorPtr mit);
  void GetOfflineMsgDoneCB(UserID uid, struct evhttp_request * req, MessageIteratorPtr mit);
 private: 
  struct event_base *evbase_;
  scoped_ptr<Storage> storage_;
 private:
  void InitAdminHttp();
  struct evhttp* admin_http_;
  static void AdminPubCB(struct evhttp_request* req, void *ctx);
  static void AdminBroadcastCB(struct evhttp_request* req, void *ctx);
  static void AdminCheckPresenceCB(struct evhttp_request* req, void * ctx);
  static void AdminCheckOffMsgCB(struct evhttp_request* req, void *ctx);
  static void AcceptErrorCB(struct evconnlistener * listener , void *ctx);
  static void SendReply(struct evhttp_request* req,  const char* content, size_t len);
 private:
  void ReplyError(struct evhttp_request* req);
  void ReplyOK(struct evhttp_request* req);
};

} // namespace xcomet


#endif
