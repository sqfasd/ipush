#ifndef SRC_SESSION_SERVER_H_
#define SRC_SESSION_SERVER_H_

#include "deps/base/singleton.h"
#include "deps/base/scoped_ptr.h"
#include "src/include_std.h"
#include "src/user.h"
#include "src/channel.h"
#include "src/router_proxy.h"
#include "src/http_query.h"

namespace xcomet {

class SessionServer {
 public:
  static SessionServer& Instance() {
    return *Singleton<SessionServer>::get();
  }

  void Connect(struct evhttp_request* req);
  void Sub(struct evhttp_request* req);
  void Pub(struct evhttp_request* req);
  void Unsub(struct evhttp_request* req);
  void Disconnect(struct evhttp_request* req);
  void Broadcast(struct evhttp_request* req);
  void RSub(struct evhttp_request* req);
  void OnTimer();
  void OnUserMessage(const string& uid, base::shared_ptr<string> message);

  void RemoveUser(User* user);

 private:
  SessionServer();
  ~SessionServer();
  void ReplyOK(struct evhttp_request* req);
  void RemoveUserFromChannel(User* user);
  void DoSub(const string& uid, const string& cid, struct evhttp_request* req);
  void DoUnsub(const string& uid, const string& cid, struct evhttp_request* req);

  UserMap users_;
  ChannelMap channels_;
  RouterProxy router_;
  UserCircleQueue timeout_queue_;

  DISALLOW_COPY_AND_ASSIGN(SessionServer);
  friend struct DefaultSingletonTraits<SessionServer>;
};

}  // namespace xcomet
#endif  // SRC_SESSION_SERVER_H_
