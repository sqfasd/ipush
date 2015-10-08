#ifndef SRC_AUTH_AUTH_H_
#define SRC_AUTH_AUTH_H_

#include "src/typedef.h"

namespace xcomet {

typedef function<void(Error, bool)> AuthCallback;

class Auth {
 public:
  virtual ~Auth() {}
  virtual void Authenticate(const string& user,
                            const string& password,
                            AuthCallback cb) = 0;
};

}  // namespace xcomet

#endif  // SRC_AUTH_AUTH_H_
