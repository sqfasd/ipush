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

class SessionServer;

class RouterProxy {
 public:
  RouterProxy(SessionServer& serv);
  ~RouterProxy();
  void SendHeartbeat();
  void Redirect(base::shared_ptr<string> message);
  void ResetSession(struct evhttp_request* req);
  void LoginUser(const string& uid);
  void LogoutUser(const string& uid);

 private:
  void OnSessionDisconnected();

  scoped_ptr<Session> session_;
  SessionServer& server_;
};
}  // namespace xcomet
#endif  // SRC_ROUTER_PROXY_H_
