#ifndef SRC_LOCAL_STORAGE_H_
#define SRC_LOCAL_STORAGE_H_

#include <event.h>
#include <inttypes.h>
#include "deps/ssdb/src/ssdb/ssdb.h"
#include "deps/base/scoped_ptr.h"
#include "src/include_std.h"
#include "src/worker.h"
#include "src/storage.h"

namespace xcomet {

class LocalStorage: public Storage {
 public:
  LocalStorage(struct event_base* evbase, const string& dir);
  virtual ~LocalStorage();
  virtual void SaveMessage(MessagePtr msg,
                           int seq,
                           SaveMessageCallback cb);
  virtual bool SaveMessageSync(MessagePtr msg, int seq);

  virtual void GetMessage(const string& uid,
                          GetMessageCallback cb);
  virtual MessageResult GetMessageSync(const string uid);

  virtual void GetMessage(const string& uid,
                          int64_t start,
                          int64_t end,
                          GetMessageCallback cb);
  virtual MessageResult GetMessageSync(const string uid,
                                       int64_t start,
                                       int64_t end);

 private:
  scoped_ptr<Worker> worker_;
  SSDB* ssdb_;
  const string dir_;
};
}  // namespace xcomet
#endif  // SRC_STORAGE_H_
