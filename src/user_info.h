#ifndef SRC_USER_INFO_H_
#define SRC_USER_INFO_H_

#include "src/include_std.h"

namespace xcomet {

class UserInfo;
typedef unordered_map<string, UserInfo> UserInfoMap;

class UserInfo {
 public:
  UserInfo(const string& uid)
    : uid_(uid),
      max_seq_(-1),
      last_ack_(-1) {
  }
  ~UserInfo() {}
  string GetId() const {return uid_;}
  int GetMaxSeq() const {return max_seq_;}
  int IncMaxSeq() {return ++max_seq_;}
  void SetMaxSeq(int seq) {max_seq_ = seq;}
  int GetLastAck() const {return last_ack_;}
  void SetLastAck(int seq) {last_ack_ = seq;}

 private:
  string uid_;
  int max_seq_;
  int last_ack_;
};

}  // namespace xcomet
#endif  // SRC_USER_INFO_H_
