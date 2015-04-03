#ifndef SRC_STORAGE_H_
#define SRC_STORAGE_H_

#include <inttypes.h>
#include "src/include_std.h"
#include "src/worker.h"
#include "src/message.h"

namespace xcomet {

typedef shared_ptr<vector<string> > UsersPtr;

typedef function<void (MessageResult)> GetMessageCallback;
typedef function<void (MessageResult)> PopMessageCallback;
typedef function<void (bool)> SaveMessageCallback;
typedef function<void (bool)> RemoveMessageCallback;
typedef function<void (int)> GetMaxSeqCallback;
typedef function<void (bool)> AddUserToChannelCallback;
typedef function<void (bool)> RemoveUserFromChannelCallback;
typedef function<void (UsersPtr)> GetChannelUsersCallback;

class Storage {
 public:
  Storage(struct event_base* evbase) : worker_(new Worker(evbase)) {}
  virtual ~Storage() {}

  virtual void SaveMessage(MessagePtr msg, int seq, SaveMessageCallback cb) {
    worker_->Do<bool>(
        bind(&Storage::SaveMessageSync, this, msg, seq), cb);
  }

  virtual bool SaveMessageSync(MessagePtr, int seq) = 0;

  virtual void GetMessage(const string& uid, GetMessageCallback cb) {
    worker_->Do<MessageResult>(
        bind(&Storage::GetMessageSync, this, uid), cb);
  }

  virtual MessageResult GetMessageSync(const string uid) = 0;

  virtual void DeleteMessage(const string& uid, RemoveMessageCallback cb) {
    worker_->Do<bool>(
        bind(&Storage::DeleteMessageSync, this, uid), cb);
  }

  virtual bool DeleteMessageSync(const string uid) = 0;

  virtual void GetMaxSeq(const string& uid, GetMaxSeqCallback cb) {
    worker_->Do<int>(
        bind(&Storage::GetMaxSeqSync, this, uid), cb);
  }

  virtual int GetMaxSeqSync(const string uid) = 0;

  virtual void UpdateAck(const string& uid,
                         int ack_seq,
                         RemoveMessageCallback cb) {
    worker_->Do<bool>(
        bind(&Storage::UpdateAckSync, this, uid, ack_seq), cb);
  }

  virtual bool UpdateAckSync(const string uid, int ack_seq) = 0;

  virtual void AddUserToChannel(const string& uid,
                                const string& cid,
                                AddUserToChannelCallback cb) {
    worker_->Do<bool>(
        bind(&Storage::AddUserToChannelSync, this, uid, cid), cb);
  }
  virtual bool AddUserToChannelSync(const string uid, const string cid) = 0;

  virtual void RemoveUserFromChannel(const string& uid,
                                     const string& cid,
                                     RemoveUserFromChannelCallback cb) {
    worker_->Do<bool>(
        bind(&Storage::RemoveUserFromChannelSync, this, uid, cid), cb);
  }
  virtual bool RemoveUserFromChannelSync(const string uid,
                                         const string cid) = 0;

  virtual void GetChannelUsers(const string& cid, GetChannelUsersCallback cb) {
    worker_->Do<UsersPtr>(
        bind(&Storage::GetChannelUsersSync, this, cid), cb);
  }
  virtual UsersPtr GetChannelUsersSync(const string cid) = 0;

 protected:
  scoped_ptr<Worker> worker_;
};
}  // namespace xcomet
#endif  // SRC_STORAGE_H_
