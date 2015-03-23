#ifndef ROUTER_SERVER_H
#define ROUTER_SERVER_H

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

#include "deps/base/shared_ptr.h"
#include "deps/base/scoped_ptr.h"
#include "deps/base/callback.h"
#include "src/remote_storage.h"
#include "src/session_proxy.h"
#include "src/utils.h"
#include "src/user_info.h"
#include "src/channel_info.h"
#include "src/typedef.h"
#include "src/stats_manager.h"

using namespace std;

namespace xcomet {

typedef map<UserID, UserInfo> UserInfoMap;
typedef map<ChannelID, ChannelInfo> ChannelInfoMap;

using base::shared_ptr;

class RouterServer {
 public:
  RouterServer(struct event_base* evbase);
  ~RouterServer();
  void Start();

 private:
  void OnSessionMsg(SessionProxy* sp, StringPtr raw_data, MessagePtr msg);
  void OnSessionProxyDisconnected(SessionProxy* sp);

  void ConnectSessionServers();

  void InitStorage();
  void InitAdminHttp();
  void InitEventHandler();

  Sid  FindSidByUid(const UserID& uid) const;
  void LoginUser(const UserID& uid, Sid sid);
  void LogoutUser(const UserID& uid) ;

  // TODO(qingfeng) use lambda or other method instead of callback
  void OnGetMaxSeqDoneToLogin(const UserID uid, Sid sid, int seq);
  void OnUpdateAckDone(bool ok);
  void OnSaveMessageDone(bool ok);
  void OnGetMsgToSend(UserID uid, MessageResult mr);
  void OnGetMsgToReply(UserID uid,
                       struct evhttp_request* req,
                       MessageResult mr);
  void OnDestroy();

  void SendAllMessages(const UserID& uid);
  void SendUserMsg(MessagePtr msg);
  void SendChannelMsg(MessagePtr msg);
  void RemoveUserFromChannel(const UserInfo& user_info);
  void Subscribe(const UserID& uid, const ChannelID& cid);
  void Unsubscribe(const UserID& uid, const ChannelID& cid);
  void UpdateUserAck(const UserID& uid, int seq);

  static void OnAdminPub(struct evhttp_request* req, void *ctx);
  static void OnAdminBroadcast(struct evhttp_request* req, void *ctx);
  static void OnAdminCheckPresence(struct evhttp_request* req, void * ctx);
  static void OnAdminCheckOffMsg(struct evhttp_request* req, void *ctx);
  static void OnAdminSub(struct evhttp_request* req, void *ctx);
  static void OnAdminUnsub(struct evhttp_request* req, void *ctx);
  static void OnAdminStats(struct evhttp_request* req, void *ctx);
  static void OnAcceptError(struct evconnlistener * listener , void *ctx);
  static void OnTimer(evutil_socket_t sig, short events, void *ctx);
  static void OnSignal(evutil_socket_t sig, short events, void *ctx);

  static void SendReply(struct evhttp_request* req, 
                        const char* content,
                        size_t len);

  static void ReplyError(struct evhttp_request* req);
  static void ReplyOK(struct evhttp_request* req, const string& resp = "");

  UserInfoMap users_;
  ChannelInfoMap channels_;
  // TODO(qingfeng) dynamic add session proxy
  vector<shared_ptr<SessionProxy> > session_proxys_;

  struct event_base *evbase_;
  struct evhttp* admin_http_;

  scoped_ptr<Storage> storage_;
  StatsManager stats_;
};

} // namespace xcomet


#endif
