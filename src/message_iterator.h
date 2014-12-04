#ifndef SRC_MESSAGE_ITERATOR_H_
#define SRC_MESSAGE_ITERATOR_H_

#include "base/logging.h"
#include "base/shared_ptr.h"
#include "src/include_std.h"

namespace xcomet {
class MessageIterator;
typedef base::shared_ptr<MessageIterator> MessageIteratorPtr;

class MessageIterator {
 public:
  MessageIterator(base::shared_ptr<queue<string> > msg_queue)
      : msgs_(msg_queue) {
    VLOG(3) << "MessageIterator construct: size=" << msgs_->size();
  }
  ~MessageIterator() {
    VLOG(3) << "MessageIterator destroy";
  }
  bool HasNext() {
    return !msgs_->empty();
  }
  std::string Next() {
    string msg = msgs_->front();
    VLOG(6) << "Next: msg=" << msg;
    msgs_->pop();
    return msg;
  }

 private:
  base::shared_ptr<queue<string> > msgs_;
};
}  // namespace xcomet

#endif  // SRC_MESSAGE_ITERATOR_H_
