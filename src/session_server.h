#ifndef SRC_SESSION_SERVER_H_
#define SRC_SESSION_SERVER_H_

#include "deps/base/singleton.h"
#include "deps/base/scoped_ptr.h"
#include "src/include_std.h"
#include "src/user.h"
#include "src/channel.h"
#include "src/router_proxy.h"
#include "src/http_query.h"
#include "src/stats_manager.h"
#include "src/typedef.h"

namespace xcomet {

class SessionServer {
 public:
  static SessionServer& Instance() {
    return *Singleton<SessionServer>::get();
  }

  void Connect(struct evhttp_request* req);
  void Pub(struct evhttp_request* req);
  void Disconnect(struct evhttp_request* req);
  void Broadcast(struct evhttp_request* req);
  void RSub(struct evhttp_request* req);
  void Stats(struct evhttp_request* req);
  void OnStart();
  void OnTimer();
  void OnUserMessage(const string& uid, base::shared_ptr<string> message);
  void OnRouterMessage(StringPtr message);

  void RemoveUser(User* user);

 private:
  SessionServer();
  ~SessionServer();
  void ReplyOK(struct evhttp_request* req, const string& resp = "");
  void RemoveUserFromChannel(User* user);
  bool IsHeartbeatMessage(StringPtr message);

  UserMap users_;
  RouterProxy router_;
  UserCircleQueue timeout_queue_;
  StatsManager stats_;
  bool register_all_;

  DISALLOW_COPY_AND_ASSIGN(SessionServer);
  friend struct DefaultSingletonTraits<SessionServer>;
};

}  // namespace xcomet
#endif  // SRC_SESSION_SERVER_H_
