#ifndef SRC_REMOTE_STORAGE_H_
#define SRC_REMOTE_STORAGE_H_

#include <event.h>
#include <inttypes.h>
#include "deps/ssdb/api/cpp/SSDB.h"
#include "deps/base/scoped_ptr.h"
#include "src/include_std.h"
#include "src/worker.h"
#include "src/storage.h"

namespace xcomet {

struct RemoteStorageOption {
  string host;
  int port;
};

class RemoteStorage: public Storage {
 public:
  RemoteStorage(struct event_base* evbase);
  RemoteStorage(struct event_base* evbase, const RemoteStorageOption& option);
  virtual ~RemoteStorage();

  virtual bool SaveMessageSync(MessagePtr msg, int seq);

  virtual MessageResult GetMessageSync(const string uid);

  virtual MessageResult GetMessageSync(const string uid,
                                       int64_t start,
                                       int64_t end);

  virtual bool UpdateAckSync(const string uid, int ack_seq);

  virtual int GetMaxSeqSync(const string uid);
 private:
  scoped_ptr<ssdb::Client> client_;
  RemoteStorageOption option_;
};

}// namesapce xcomet

#endif
