#ifndef SRC_REMOTE_STORAGE_H_
#define SRC_REMOTE_STORAGE_H_

#include <event.h>
#include <inttypes.h>
#include "deps/ssdb/src/ssdb/ssdb.h"
#include "deps/base/scoped_ptr.h"
#include "src/include_std.h"
#include "src/message_iterator.h"
#include "src/worker.h"
#include "SSDB.h"
#include "storage.h"

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
  virtual void SaveMessage(const string& uid,
                          const string& content,
                          boost::function<void (bool)> cb);
  virtual bool SaveMessageSync(const string uid, const string content);

  virtual void PopMessageIterator(const string& uid,
                                 boost::function<void (MessageIteratorPtr)> cb);
  virtual MessageIteratorPtr PopMessageIteratorSync(const string uid);

  virtual void GetMessageIterator(const string& uid,
                                 boost::function<void (MessageIteratorPtr)> cb);
  virtual MessageIteratorPtr GetMessageIteratorSync(const string uid);

  virtual void GetMessageIterator(const string& uid, int64_t start, int64_t end, 
                                boost::function<void (MessageIteratorPtr)> cb);
  
  virtual MessageIteratorPtr GetMessageIteratorSync(const string uid, int64_t start, int64_t end);

  //void RemoveMessages(const string& uid, base::Callback1<bool>* cb);
  //bool RemoveMessagesSync(const string& uid);

 private:
  scoped_ptr<Worker> worker_;
  scoped_ptr<ssdb::Client> client_;
  RemoteStorageOption option_;
};

}// namesapce xcomet

#endif
