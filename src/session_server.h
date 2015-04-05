#ifndef SRC_SESSION_SERVER_H_
#define SRC_SESSION_SERVER_H_

#include "deps/base/scoped_ptr.h"
#include "deps/base/concurrent_queue.h"
#include "src/include_std.h"
#include "src/user.h"
#include "src/user_info.h"
#include "src/http_query.h"
#include "src/stats_manager.h"
#include "src/typedef.h"
#include "src/evhelper.h"
#include "src/message.h"

namespace xcomet {

class SessionServerPrivate;
class SessionServer {
 public:
  SessionServer();
  ~SessionServer();
  void Start();
  void Stop();

  void Connect(struct evhttp_request* req);
  void Pub(struct evhttp_request* req);
  void Disconnect(struct evhttp_request* req);
  void Broadcast(struct evhttp_request* req);
  void Stats(struct evhttp_request* req);
  void Sub(struct evhttp_request* req);
  void Unsub(struct evhttp_request* req);
  void Msg(struct evhttp_request* req);

  void OnTimer();
  void OnUserMessage(const string& uid, shared_ptr<string> message);
  void OnRouterMessage(StringPtr message);
  void OnUserDisconnect(User* user);

  void RunInNextTick(function<void ()> fn);

 private:
  DISALLOW_COPY_AND_ASSIGN(SessionServer);

  void SetupClientHandler();
  void SetupAdminHandler();
  void SetupEventHandler();

  void OnStart();
  void OnStop();

  bool IsHeartbeatMessage(const string& message);
  void SendUserMsg(MessagePtr msg);
  void SendChannelMsg(MessagePtr msg);

  SessionServerPrivate* p_;
  const int timeout_counter_;
  UserMap users_;
  UserInfoMap user_infos_;
  UserCircleQueue timeout_queue_;
  StatsManager stats_;
  base::ConcurrentQueue<function<void ()> > task_queue_;
};

}  // namespace xcomet
#endif  // SRC_SESSION_SERVER_H_
