#ifndef SRC_AUTH_H_
#define SRC_AUTH_H_

#include "deps/base/lru_cache-inl.h"
#include "src/include_std.h"
#include "src/typedef.h"

namespace xcomet {

typedef function<void(ErrorPtr, bool)> AuthCallback;

class Auth {
 public:
  Auth();
  ~Auth();
  void Authenticate(const string& user,
                    const string& password,
                    AuthCallback cb);

 private:
  bool RSAPrivateDecode(const string& input, string& output);
  string private_key_;
  base::LRUCache<string, bool> cache_;
  enum Type {
    T_NONE,
    T_FAST,
    T_FULL,
  } type_;
};

}  // namespace xcomet

#endif  // SRC_AUTH_H_
