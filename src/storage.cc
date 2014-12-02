#include "src/storage.h"

#include "deps/base/logging.h"

namespace xcomet {

Storage::Storage(struct event_base* evbase, const string& dir)
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

Storage::~Storage() {
  worker_.reset();
  delete ssdb_;
}

void Storage::SaveOfflineMessage(
    const string& uid,
    const string& content,
    boost::function<void (bool)> cb) {
  worker_->Do<bool>(boost::bind(&Storage::SaveOfflineMessageSync, this, uid, content), cb);
}

bool Storage::SaveOfflineMessageSync(const string uid, const string content) {
  ssdb_->qpush_back(Bytes(uid), Bytes(content));
  return true;
}

void Storage::PopOfflineMessageIterator(
    const string& uid,
    boost::function<void (MessageIteratorPtr)> cb) {
  worker_->Do<MessageIteratorPtr>(boost::bind(&Storage::PopOfflineMessageIteratorSync, this, uid), cb);
}

MessageIteratorPtr Storage::PopOfflineMessageIteratorSync(const string uid) {
  string str1;
  base::shared_ptr<queue<string> > mq(new queue<string>());
  while (ssdb_->qpop_front(uid, &str1) > 0) {
    mq->push(str1);
  }
  return MessageIteratorPtr(new MessageIterator(mq));
}

void Storage::GetOfflineMessageIterator(
    const string& uid,
    boost::function<void (MessageIteratorPtr)> cb) {
  worker_->Do<MessageIteratorPtr>(boost::bind(&Storage::GetOfflineMessageIteratorSync, this, uid), cb);
}

MessageIteratorPtr Storage::GetOfflineMessageIteratorSync(const string uid) {
  vector<string> result;
  // TODO avoid inefficient copy
  ssdb_->qslice(uid, 0, -1, &result);
  base::shared_ptr<queue<string> > mq(new queue<string>());
  for (size_t i = 0; i < result.size(); ++i) {
    mq->push(result[i]);
  }
  return MessageIteratorPtr(new MessageIterator(mq));
}

void Storage::GetMessageIterator(
    const string& uid,
    int64_t start,
    int64_t end,
    boost::function<void (MessageIteratorPtr)> cb) {
  worker_->Do<MessageIteratorPtr>(boost::bind(&Storage::GetMessageIteratorSync, this, uid, start, end), cb);
}

MessageIteratorPtr Storage::GetMessageIteratorSync(const string uid, int64_t start, int64_t end) {
  vector<string> result;
  ssdb_->qslice(uid, start, end, &result);
  base::shared_ptr<queue<string> > mq(new queue<string>());
  for(size_t i = 0; i < result.size(); ++i ) {
    mq->push(result[i]);
  }
  return MessageIteratorPtr(new MessageIterator(mq));
}

}  // namespace xcomet
