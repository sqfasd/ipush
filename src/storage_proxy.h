#ifndef SRC_STORAGE_PROXY_H_
#define SRC_STORAGE_PROXY_H_

#include "src/include_std.h"
#include "src/router_proxy.h"

namespace xcomet {

class StorageProxy {
 public:
  StorageProxy(RouterProxy& router) : router_(router) {
  }
  ~StorageProxy() {
  }
  bool HasOfflineMessage(const std::string& uid) {
    return router_.HasOfflineMessage(uid);;
  }
  MessageIterator GetOfflineMessageIterator(const string& uid) {
    return MessageIterator(router_.GetOfflieMessages(uid));
  }
  void RemoveOfflineMessages(const string& uid) {
    router_.RemoveOfflineMessages(uid);
  }
https://github.com/sqfasd/ipractice/blob/master/net/libevent/http_client.cc
 private:
  RouterProxy& router_;
};
}  // namespace xcomet
#endif  // SRC_STORAGE_PROXY_H_
