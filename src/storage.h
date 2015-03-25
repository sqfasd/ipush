#ifndef SRC_STORAGE_H_
#define SRC_STORAGE_H_

#include <inttypes.h>
#include "deps/base/scoped_ptr.h"
#include "deps/base/shared_ptr.h"
#include "src/include_std.h"
#include "src/worker.h"
#include "src/message.h"

namespace xcomet {

typedef base::shared_ptr<vector<string> > UsersPtr;

typedef boost::function<void (MessageResult)> GetMessageCallback;
typedef boost::function<void (MessageResult)> PopMessageCallback;
typedef boost::function<void (bool)> SaveMessageCallback;
typedef boost::function<void (bool)> RemoveMessageCallback;
typedef boost::function<void (int)> GetMaxSeqCallback;
typedef boost::function<void (bool)> AddUserToChannelCallback;
typedef boost::function<void (bool)> RemoveUserFromChannelCallback;
typedef boost::function<void (UsersPtr)> GetChannelUsersCallback;

class Storage {
 public:
  Storage(struct event_base* evbase) : worker_(new Worker(evbase)) {}
  virtual ~Storage() {}

  virtual void SaveMessage(MessagePtr msg, int seq, SaveMessageCallback cb) {
    worker_->Do<bool>(
        boost::bind(&Storage::SaveMessageSync, this, msg, seq), cb);
  }

  virtual bool SaveMessageSync(MessagePtr, int seq) = 0;

  virtual void GetMessage(const string& uid, GetMessageCallback cb) {
    worker_->Do<MessageResult>(
        boost::bind(&Storage::GetMessageSync, this, uid), cb);
  }

  virtual MessageResult GetMessageSync(const string uid) = 0;

  virtual void DeleteMessage(const string& uid, RemoveMessageCallback cb) {
    worker_->Do<bool>(
        boost::bind(&Storage::DeleteMessageSync, this, uid), cb);
  }

  virtual bool DeleteMessageSync(const string uid) = 0;

  virtual void GetMaxSeq(const string& uid, GetMaxSeqCallback cb) {
    worker_->Do<int>(
        boost::bind(&Storage::GetMaxSeqSync, this, uid), cb);
  }

  virtual int GetMaxSeqSync(const string uid) = 0;

  virtual void UpdateAck(const string& uid,
                         int ack_seq,
                         RemoveMessageCallback cb) {
    worker_->Do<bool>(
        boost::bind(&Storage::UpdateAckSync, this, uid, ack_seq), cb);
  }

  virtual bool UpdateAckSync(const string uid, int ack_seq) = 0;

  virtual void AddUserToChannel(const string& uid,
                                const string& cid,
                                AddUserToChannelCallback cb) {
    worker_->Do<bool>(
        boost::bind(&Storage::AddUserToChannelSync, this, uid, cid), cb);
  }
  virtual bool AddUserToChannelSync(const string uid, const string cid) = 0;

  virtual void RemoveUserFromChannel(const string& uid,
                                     const string& cid,
                                     RemoveUserFromChannelCallback cb) {
    worker_->Do<bool>(
        boost::bind(&Storage::RemoveUserFromChannelSync, this, uid, cid), cb);
  }
  virtual bool RemoveUserFromChannelSync(const string uid,
                                         const string cid) = 0;

  virtual void GetChannelUsers(const string& cid, GetChannelUsersCallback cb) {
    worker_->Do<UsersPtr>(
        boost::bind(&Storage::GetChannelUsersSync, this, cid), cb);
  }
  virtual UsersPtr GetChannelUsersSync(const string cid) = 0;

 protected:
  scoped_ptr<Worker> worker_;
};
}  // namespace xcomet
#endif  // SRC_STORAGE_H_
