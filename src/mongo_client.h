#ifndef SRC_MONGO_CLIENT_H_
#define SRC_MONGO_CLIENT_H_

#include "deps/base/scoped_ptr.h"
#include "src/include_std.h"
#include "src/typedef.h"

namespace xcomet {

typedef function<void(Error, StringPtr)> GetUserNameCallback;

class MongoClientPrivate;
class MongoClient {
 public:
  MongoClient();
  ~MongoClient();
  void GetUserNameByDevId(const StringPtr& devid,
                          const GetUserNameCallback& cb);
 private:
  scoped_ptr<MongoClientPrivate> p_;
};

}  // namespace xcomet
#endif  // SRC_MONGO_CLIENT_H_
