#ifndef SRC_AUTH_AUTH_PROXY_H_
#define SRC_AUTH_AUTH_PROXY_H_

#include <event.h>
#include "deps/base/lru_cache-inl.h"
#include "src/include_std.h"
#include "src/auth/auth.h"

namespace xcomet {

class AuthProxy : public Auth {
 public:
  AuthProxy(struct event_base* evbase);
  virtual ~AuthProxy();
  virtual void Authenticate(const string& user,
                            const string& password,
                            AuthCallback cb);
 private:
  struct event_base* evbase_;
  base::LRUCache<string, bool> cache_;
  string proxy_ip_;
  int proxy_port_;
};

}  // namespace xcomet

#endif  // SRC_AUTH_AUTH_PROXY_H_
