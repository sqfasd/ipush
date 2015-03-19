#ifndef SRC_USER_INFO_H_
#define SRC_USER_INFO_H_

#include "src/include_std.h"

namespace xcomet {

class UserInfo {
 public:
  UserInfo(const string& uid) :uid_(uid) {}
  ~UserInfo() {}
  string GetId() const {return uid_;}
  const set<string>& GetChannelIds() const {return channel_ids_;}
  void SubChannel(const string& cid) {channel_ids_.insert(cid);}
  void UnsubChannel(const string& cid) {channel_ids_.erase(cid);}
  string GetId() const {return uid_;}

 private:
  string uid_;
  set<string> channel_ids_;
};

}  // namespace xcomet
#endif  // SRC_USER_INFO_H_
