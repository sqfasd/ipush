#ifndef SRC_AUTH_AUTH_DB_H_
#define SRC_AUTH_AUTH_DB_H_

#include "deps/base/threadsafe_lru_cache-inl.h"
#include "deps/base/scoped_ptr.h"
#include "src/include_std.h"
#include "src/typedef.h"
#include "src/auth/auth.h"

namespace xcomet {

class MongoClient;
class AuthDB : public Auth {
 public:
  AuthDB();
  virtual ~AuthDB();
  virtual void Authenticate(const string& user,
                            const string& password,
                            AuthCallback cb);

 private:
  bool RSAPrivateDecode(const string& input, string& output);
  string private_key_;
  base::ThreadSafeLRUCache<string, bool> cache_;
  scoped_ptr<MongoClient> mongo_;
  enum Type {
    T_NONE,
    T_FAST,
    T_FULL,
  } type_;
  vector<string> device_id_fields_;
};

}  // namespace xcomet

#endif  // SRC_AUTH_AUTH_DB_H_
