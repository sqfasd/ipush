#ifndef SRC_CHANNEL_INFO_H_
#define SRC_CHANNEL_INFO_H_

namespace xcomet {

class ChannelInfo {
 public:
  ChannelInfo(const string& id) :id_(id) {
  }
  ~ChannelInfo() {}
  string GetId() const {return id_;}
  int GetUserCount() const {return users_.size();}
  void AddUser(const string& uid) {users_.insert(uid)}
  void RemoveUser(User* user) {users_.erase(uid);}

 private:
  string id_;
  set<string> users_;
};

}  // namespace xcomet
#endif  // SRC_CHANNEL_INFO_H_
