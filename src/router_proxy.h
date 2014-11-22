#ifndef SRC_ROUTER_PROXY_H_
#define SRC_ROUTER_PROXY_H_

#include "src/include_std.h"
#include "base/logging.h"

namespace xcomet {

class RouterProxy {
  typedef map<string, queue<string> > MsgMap;
 public:
  RouterProxy() {
  }
  ~RouterProxy() {
  }
  void Redirect(const string& uid, const string& content) {
    if (users_.find(uid) != users_.end()) {
      LOG(INFO) << "[FIXME] Redirect: uid="
                << uid << ", content=" << content;
    } else {
      LOG(INFO) << "[FIXME] save offline msg: uid=" << uid
                << ", content" << content;
      user_msgs_[uid].push(content);
    }
  }
  void RegisterUser(const string& uid) {
    LOG(INFO) << "[FIXME] RegisterUser: " << uid;
    users_.insert(uid);
  }
  void UnregisterUser(const string& uid) {
    users_.erase(uid);
  }
  void ChannelBroadcast(const string& cid, const string& content) {
    LOG(INFO) << "[FIXME] ChannelBroadcast: cid="
              << cid << ", content=" << content;
  }
  queue<string>* GetOfflieMessages(const string& uid) {
    return &user_msgs_[uid];
  }
  bool HasOfflineMessage(const string& uid) {
    MsgMap::iterator iter = user_msgs_.find(uid);
    if (iter != user_msgs_.end()) {
      return !iter->second.empty();
    }
    return false;
  }
  void RemoveOfflineMessages(const string& uid) {
    user_msgs_.erase(uid);
  }
 private:
  MsgMap user_msgs_;
  set<string> users_;
};
}  // namespace xcomet
#endif  // SRC_ROUTER_PROXY_H_
