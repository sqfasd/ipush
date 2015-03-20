#ifndef SRC_USER_INFO_H_
#define SRC_USER_INFO_H_

#include "src/include_std.h"

namespace xcomet {

class UserInfo {
 public:
  UserInfo(const string& uid, int ssid)
    : uid_(uid),
      ssid_(ssid),
      max_seq_(-1),
      last_ack_(-1) {
  }
  ~UserInfo() {}
  string GetId() const {return uid_;}
  const set<string>& GetChannelIds() const {return channel_ids_;}
  void SubChannel(const string& cid) {channel_ids_.insert(cid);}
  void UnsubChannel(const string& cid) {channel_ids_.erase(cid);}
  int GetSSId() const {return ssid_;}
  int GetMaxSeq() const {return max_seq_;}
  int IncMaxSeq() {return ++max_seq_;}
  void SetMaxSeq(int seq) {max_seq_ = seq;}
  int GetLastAck() const {return last_ack_;}
  void SetLastAck(int seq) {last_ack_ = seq;}

 private:
  string uid_;
  int ssid_;  // session server id
  set<string> channel_ids_;
  int max_seq_;
  int last_ack_;
};

}  // namespace xcomet
#endif  // SRC_USER_INFO_H_
