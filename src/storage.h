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
  virtual ~Storage() {};
  virtual void SaveMessage(const string& uid,
                          const string& content,
                          boost::function<void (bool)> cb) = 0;
  virtual bool SaveMessageSync(const string uid, const string content) = 0;

  virtual void PopMessageIterator(const string& uid,
                                 boost::function<void (MessageIteratorPtr)> cb) = 0;
  virtual MessageIteratorPtr PopMessageIteratorSync(const string uid) = 0;

  virtual void GetMessageIterator(const string& uid,
                                 boost::function<void (MessageIteratorPtr)> cb) = 0;
  virtual MessageIteratorPtr GetMessageIteratorSync(const string uid) = 0;

  virtual void GetMessageIterator(const string& uid, int64_t start, int64_t end, 
                                boost::function<void (MessageIteratorPtr)> cb) = 0;
  
  virtual MessageIteratorPtr GetMessageIteratorSync(const string uid, int64_t start, int64_t end) = 0;
};
}  // namespace xcomet
#endif  // SRC_STORAGE_H_
