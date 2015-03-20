#include "src/local_storage.h"

#include "deps/base/logging.h"

namespace xcomet {

LocalStorage::LocalStorage(struct event_base* evbase, const string& dir)
    : worker_(new Worker(evbase)),
      ssdb_(NULL),
      dir_(dir) {
  Options opt;
  opt.cache_size = 500;
  opt.block_size = 32;
  opt.write_buffer_size = 64;
  opt.compaction_speed = 1000;
  opt.compression = "no";
  ssdb_ = SSDB::open(opt, dir_);
  CHECK(ssdb_) << "open ssdb failed";
}

LocalStorage::~LocalStorage() {
  worker_.reset();
  delete ssdb_;
}

void LocalStorage::SaveMessage(
    MessagePtr msg,
    int seq,
    SaveMessageCallback cb) {
  worker_->Do<bool>(
      boost::bind(&LocalStorage::SaveMessageSync, this, msg, seq), cb);
}

bool LocalStorage::SaveMessageSync(MessagePtr msg, int seq) {
  return true;
}

void LocalStorage::GetMessage(
    const string& uid,
    GetMessageCallback cb) {
  worker_->Do<MessageResult>(
      boost::bind(&LocalStorage::GetMessageSync, this, uid), cb);
}

MessageResult LocalStorage::GetMessageSync(const string uid) {
  MessageResult result(new vector<string>());
  result->reserve(DEFAULT_BATCH_GET_SIZE);
  ssdb_->qslice(uid, 0, -1, result.get());
  return result;
}

void LocalStorage::GetMessage(
    const string& uid,
    int64_t start,
    int64_t end,
    GetMessageCallback cb) {
  worker_->Do<MessageResult>(
      boost::bind(&LocalStorage::GetMessageSync,
          this, uid, start, end),
      cb);
}

MessageResult LocalStorage::GetMessageSync(
    const string uid,
    int64_t start,
    int64_t end) {
  VLOG(5) << "qslice " << start << "," << end;
  MessageResult result(new vector<string>());
  result->reserve(DEFAULT_BATCH_GET_SIZE);
  ssdb_->qslice(uid, start, end, result.get());
  return result;
}

}  // namespace xcomet
