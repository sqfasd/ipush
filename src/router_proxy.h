#ifndef SRC_ROUTER_PROXY_H_
#define SRC_ROUTER_PROXY_H_

#include <evhttp.h>
#include "deps/base/logging.h"
#include "deps/base/scoped_ptr.h"
#include "deps/base/shared_ptr.h"
#include "deps/base/callback.h"
#include "src/include_std.h"
#include "src/session.h"

namespace xcomet {

class RouterProxy {
 public:
  RouterProxy();
  ~RouterProxy();
  void SendHeartbeat();
  void ResetSession(struct evhttp_request* req);
  void RegisterUser(const string& uid, int seq);
  void UnregisterUser(const string& uid);

  void SetCounter(int n) {counter_ = n;}
  int IncCounter() {return ++counter_;}
 private:
  void OnSessionDisconnected();

  scoped_ptr<Session> session_;
  int counter_;
};
}  // namespace xcomet
#endif  // SRC_ROUTER_PROXY_H_
