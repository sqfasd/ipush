#ifndef SRC_STORAGE_H_
#define SRC_STORAGE_H_

#include <event.h>
#include <inttypes.h>
#include "deps/ssdb/src/ssdb/ssdb.h"
#include "deps/base/scoped_ptr.h"
#include "src/include_std.h"
#include "src/message_iterator.h"
#include "src/worker.h"

namespace xcomet {

class Storage {
 public:
  Storage(struct event_base* evbase, const string& dir);
  ~Storage();
  void SaveOfflineMessage(const string& uid,
                          const string& content,
                          boost::function<void (bool)> cb);
  bool SaveOfflineMessageSync(const string uid, const string content);

  void PopOfflineMessageIterator(const string& uid,
                                 boost::function<void (MessageIteratorPtr)> cb);
  MessageIteratorPtr PopOfflineMessageIteratorSync(const string uid);

  void GetOfflineMessageIterator(const string& uid,
                                 boost::function<void (MessageIteratorPtr)> cb);
  MessageIteratorPtr GetOfflineMessageIteratorSync(const string uid);

  void GetMessageIterator(const string& uid, int64_t start, int64_t end, 
                                boost::function<void (MessageIteratorPtr)> cb);
  
  MessageIteratorPtr GetMessageIteratorSync(const string uid, int64_t start, int64_t end);

  //void RemoveOfflineMessages(const string& uid, base::Callback1<bool>* cb);
  //bool RemoveOfflineMessagesSync(const string& uid);

 private:
  scoped_ptr<Worker> worker_;
  SSDB* ssdb_;
  const string dir_;
};
}  // namespace xcomet
#endif  // SRC_STORAGE_H_
