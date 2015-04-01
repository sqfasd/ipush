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

  // offline messages
  virtual bool SaveMessageSync(MessagePtr msg, int seq);
  virtual MessageResult GetMessageSync(const string uid);
  virtual bool DeleteMessageSync(const string uid);
  virtual int GetMaxSeqSync(const string uid);
  virtual bool UpdateAckSync(const string uid, int ack_seq);

  // channels
  virtual bool AddUserToChannelSync(const string uid, const string cid);
  virtual bool RemoveUserFromChannelSync(const string uid, const string cid);
  virtual UsersPtr GetChannelUsersSync(const string cid);

 private:
  bool UpdateKey(const string& uid, const string& key, const string& value);
  bool GetSeqs(const string& uid, int& min_seq, int& last_ack);

  scoped_ptr<ssdb::Client> client_;
  RemoteStorageOption option_;
};

}// namesapce xcomet

#endif
