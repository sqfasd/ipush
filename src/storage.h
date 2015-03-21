#ifndef SRC_STORAGE_H_
#define SRC_STORAGE_H_

#include <inttypes.h>
#include "deps/base/scoped_ptr.h"
#include "deps/base/shared_ptr.h"
#include "src/include_std.h"
#include "src/worker.h"
#include "src/message.h"

namespace xcomet {

const int DEFAULT_BATCH_GET_SIZE = 100;
typedef boost::function<void (MessageResult)> GetMessageCallback;
typedef boost::function<void (MessageResult)> PopMessageCallback;
typedef boost::function<void (bool)> SaveMessageCallback;
typedef boost::function<void (bool)> RemoveMessageCallback;
typedef boost::function<void (int)> GetMaxSeqCallback;

class Storage {
 public:
  Storage(struct event_base* evbase) : worker_(new Worker(evbase)) {}
  virtual ~Storage() {}

  void SaveMessage(MessagePtr msg, int seq, SaveMessageCallback cb) {
    worker_->Do<bool>(
        boost::bind(&Storage::SaveMessageSync, this, msg, seq), cb);
  }

  virtual bool SaveMessageSync(MessagePtr, int seq) = 0;

  void GetMessage(const string& uid, GetMessageCallback cb) {
    worker_->Do<MessageResult>(
        boost::bind(&Storage::GetMessageSync, this, uid), cb);
  }

  virtual MessageResult GetMessageSync(const string uid) = 0;

  void GetMessage(const string& uid,
                  int64_t start,
                  int64_t end,
                  GetMessageCallback cb) {
    worker_->Do<MessageResult>(
        boost::bind(&Storage::GetMessageSync,
                    this, uid, start, end),
        cb);
  }

  virtual MessageResult GetMessageSync(const string uid,
                                       int64_t start,
                                       int64_t end) = 0;

  void UpdateAck(const string& uid, int ack_seq, RemoveMessageCallback cb) {
    worker_->Do<bool>(
        boost::bind(&Storage::UpdateAckSync, this, uid, ack_seq), cb);
  }

  virtual bool UpdateAckSync(const string uid, int ack_seq) = 0;

  void GetMaxSeq(const string& uid, GetMaxSeqCallback cb) {
    worker_->Do<int>(
        boost::bind(&Storage::GetMaxSeqSync, this, uid), cb);
  }

  virtual int GetMaxSeqSync(const string uid) = 0;

 protected:
  scoped_ptr<Worker> worker_;
};
}  // namespace xcomet
#endif  // SRC_STORAGE_H_
