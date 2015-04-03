#ifndef SRC_SESSION_SERVER_H_
#define SRC_SESSION_SERVER_H_

#include "deps/base/singleton.h"
#include "deps/base/scoped_ptr.h"
#include "deps/base/concurrent_queue.h"
#include "src/include_std.h"
#include "src/user.h"
#include "src/http_query.h"
#include "src/stats_manager.h"
#include "src/typedef.h"
#include "src/evhelper.h"

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
  void Stats(struct evhttp_request* req);
  void OnStart();
  void OnTimer();
  void OnUserMessage(const string& uid, base::shared_ptr<string> message);
  void OnRouterMessage(StringPtr message);
  void OnUserDisconnect(User* user);
  void RunInNextTick(boost::function<void ()> fn);

 private:
  SessionServer();
  ~SessionServer();
  bool IsHeartbeatMessage(const string& message);

  const int timeout_counter_;
  UserMap users_;
  UserCircleQueue timeout_queue_;
  StatsManager stats_;
  base::ConcurrentQueue<boost::function<void ()> > task_queue_;

  DISALLOW_COPY_AND_ASSIGN(SessionServer);
  friend struct DefaultSingletonTraits<SessionServer>;
};

}  // namespace xcomet
#endif  // SRC_SESSION_SERVER_H_
