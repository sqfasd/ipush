#ifndef SRC_STORAGE_H_
#define SRC_STORAGE_H_

#include "src/include_std.h"
#include "src/router_proxy.h"

namespace xcomet {

class MessageIterator {
 public:
  MessageIterator(queue<string>* msg_queue) : msgs_(msg_queue) {
    VLOG(3) << "MessageIterator: size=" << msgs_->size();
  }
  ~MessageIterator() {}
  bool HasNext() {
    return !msgs_->empty();
  }
  std::string Next() {
    string msg = msgs_->front();
    VLOG(3) << "Next: msg=" << msg;
    msgs_->pop();
    return msg;
  }

 private:
  queue<string>* msgs_;
};

class Storage {
 public:
  Storage(RouterProxy& router) : router_(router) {
  }
  ~Storage() {
  }
  bool HasOfflineMessage(const std::string& uid) {
    return router_.HasOfflineMessage(uid);;
  }
  MessageIterator GetOfflineMessageIterator(
      const std::string& uid) {
    return MessageIterator(router_.GetOfflieMessages(uid));
  }
  void RemoveOfflineMessages(const string& uid) {
    router_.RemoveOfflineMessages(uid);
  }

 private:
  RouterProxy& router_;
};
}  // namespace xcomet
#endif  // SRC_STORAGE_H_
