#ifndef SRC_INMEMORY_STORAGE_H_
#define SRC_INMEMORY_STORAGE_H_

#include "deps/jsoncpp/include/json/value.h"
#include "src/storage.h"

namespace xcomet {

class InMemoryUserData {
 public:
  InMemoryUserData();
  ~InMemoryUserData();
  void AddMessage(const StringPtr& msg, int64 ttl);
  MessageDataSet GetMessages();
  void SetAck(int ack) {ack_ = ack;}
  int GetMaxSeq() {return tail_seq_;}
  bool Dump(Json::Value& json);
  void Load(const Json::Value& json);

 private:
  void GetQueueInfo(int& start, int& len);
  bool IsMsgOK(int64 now, const pair<int64, StringPtr>& msg);

  int head_;
  int head_seq_;
  int tail_;
  int tail_seq_;
  int ack_;
  // pair is expired_second + message
  vector<pair<int64, StringPtr> > msg_queue_;
};

class InMemoryStorage : public Storage {
 public:
  InMemoryStorage();
  ~InMemoryStorage();
 private:
  virtual void SaveMessage(const StringPtr& msg,
                           const string& uid,
                           int64 ttl,
                           SaveMessageCallback cb);
  virtual void GetMessage(const string& uid, GetMessageCallback cb);
  virtual void GetMaxSeq(const string& uid, GetMaxSeqCallback cb);
  virtual void UpdateAck(const string& uid,
                         int ack_seq,
                         UpdateAckCallback cb);

  virtual void AddUserToChannel(const string& uid,
                                const string& cid,
                                AddUserToChannelCallback cb);
  virtual void RemoveUserFromChannel(const string& uid,
                                     const string& cid,
                                     RemoveUserFromChannelCallback cb);
  virtual void GetChannelUsers(const string& cid,
                               GetChannelUsersCallback cb);
  void Dump();
  void Load();

  map<string, InMemoryUserData> user_data_;
  map<string, set<string> > channel_map_;
};

}  // namespace xcomet

#endif  // SRC_INMEMORY_STORAGE_H_
