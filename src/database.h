#ifndef SRC_DATABASE_H_
#define SRC_DATABASE_H_

#include <event.h>
#include <inttypes.h>
#include "deps/ssdb/src/ssdb/ssdb.h"
#include "deps/base/scoped_ptr.h"
#include "src/include_std.h"
#include "src/message_iterator.h"
#include "src/worker.h"
#include "SSDB.h"

namespace xcomet {

struct DatabaseOption {
  string host;
  int port;
};

class Database {
 public:
  Database(struct event_base* evbase);
  Database(struct event_base* evbase, const DatabaseOption& option);
  ~Database();
  void SaveMessage(const string& uid,
                          const string& content,
                          boost::function<void (bool)> cb);
  bool SaveMessageSync(const string uid, const string content);

  void GetMessageIterator(const string& uid,
                                 boost::function<void (MessageIteratorPtr)> cb);
  MessageIteratorPtr GetMessageIteratorSync(const string uid);

  void GetMessageIterator(const string& uid, int64_t start, int64_t end, 
                                boost::function<void (MessageIteratorPtr)> cb);
  
  MessageIteratorPtr GetMessageIteratorSync(const string uid, int64_t start, int64_t end);

  //void RemoveMessages(const string& uid, base::Callback1<bool>* cb);
  //bool RemoveMessagesSync(const string& uid);

 private:
  scoped_ptr<Worker> worker_;
  scoped_ptr<ssdb::Client> client_;
  DatabaseOption option_;
};

}// namesapce xcomet

#endif
