#ifndef SRC_USER_INFO_H_
#define SRC_USER_INFO_H_

#include "src/include_std.h"

namespace xcomet {

class UserInfo {
 public:
  UserInfo(const string& uid, int sid)
    : uid_(uid),
      sid_(sid),
      max_seq_(0),
      last_ack_(0),
      online_(true) {
  }
  ~UserInfo() {}
  string GetId() const {return uid_;}
  const set<string>& GetChannelIds() const {return channel_ids_;}
  void SubChannel(const string& cid) {channel_ids_.insert(cid);}
  void UnsubChannel(const string& cid) {channel_ids_.erase(cid);}
  int GetSid() const {return sid_;}
  void SetSid(int sid) {sid_ = sid;}
  int GetMaxSeq() const {return max_seq_;}
  int IncMaxSeq() {return ++max_seq_;}
  void SetMaxSeq(int seq) {max_seq_ = seq;}
  int GetLastAck() const {return last_ack_;}
  void SetLastAck(int seq) {last_ack_ = seq;}
  bool IsOnline() const {return online_;}
  void SetOnline(bool online) {online_ = online;}

 private:
  string uid_;
  int sid_;  // session server id
  set<string> channel_ids_;
  int max_seq_;
  int last_ack_;
  bool online_;
};

}  // namespace xcomet
#endif  // SRC_USER_INFO_H_
