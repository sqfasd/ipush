#ifndef SRC_CHANNEL_INFO_H_
#define SRC_CHANNEL_INFO_H_

#include "src/include_std.h"

namespace xcomet {

class ChannelInfo;
typedef map<string, ChannelInfo> ChannelInfoMap;

class ChannelInfo {
 public:
  ChannelInfo(const string& id) :id_(id) {
  }
  ~ChannelInfo() {}
  string GetId() const {return id_;}
  int GetUserCount() const {return users_.size();}
  void AddUser(const string& uid) {users_.insert(uid);}
  void RemoveUser(const string& uid) {users_.erase(uid);}
  const set<string>& GetUsers() const {return users_;}

 private:
  string id_;
  // TODO(qingfeng) use pointers
  set<string> users_;
};

}  // namespace xcomet
#endif  // SRC_CHANNEL_INFO_H_
