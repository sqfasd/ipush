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
    const string& uid,
    const string& content,
    boost::function<void (bool)> cb) {
  worker_->Do<bool>(boost::bind(&LocalStorage::SaveMessageSync, this, uid, content), cb);
}

bool LocalStorage::SaveMessageSync(const string uid, const string content) {
  ssdb_->qpush_back(Bytes(uid), Bytes(content));
  return true;
}

void LocalStorage::PopMessageIterator(
    const string& uid,
    boost::function<void (MessageIteratorPtr)> cb) {
  worker_->Do<MessageIteratorPtr>(boost::bind(&LocalStorage::PopMessageIteratorSync, this, uid), cb);
}

MessageIteratorPtr LocalStorage::PopMessageIteratorSync(const string uid) {
  string str1;
  base::shared_ptr<queue<string> > mq(new queue<string>());
  while (ssdb_->qpop_front(uid, &str1) > 0) {
    mq->push(str1);
  }
  return MessageIteratorPtr(new MessageIterator(mq));
}

void LocalStorage::GetMessageIterator(
    const string& uid,
    boost::function<void (MessageIteratorPtr)> cb) {
  worker_->Do<MessageIteratorPtr>(boost::bind(&LocalStorage::GetMessageIteratorSync, this, uid), cb);
}

MessageIteratorPtr LocalStorage::GetMessageIteratorSync(const string uid) {
  vector<string> result;
  // TODO avoid inefficient copy
  ssdb_->qslice(uid, 0, -1, &result);
  base::shared_ptr<queue<string> > mq(new queue<string>());
  for (size_t i = 0; i < result.size(); ++i) {
    mq->push(result[i]);
  }
  return MessageIteratorPtr(new MessageIterator(mq));
}

void LocalStorage::GetMessageIterator(
    const string& uid,
    int64_t start,
    int64_t end,
    boost::function<void (MessageIteratorPtr)> cb) {
  worker_->Do<MessageIteratorPtr>(boost::bind(&LocalStorage::GetMessageIteratorSync, this, uid, start, end), cb);
}

MessageIteratorPtr LocalStorage::GetMessageIteratorSync(const string uid, int64_t start, int64_t end) {
  vector<string> result;
  ssdb_->qslice(uid, start, end, &result);
  VLOG(5) << "qslice " << start << "," << end;
  base::shared_ptr<queue<string> > mq(new queue<string>());
  for(size_t i = 0; i < result.size(); ++i ) {
    mq->push(result[i]);
  }
  return MessageIteratorPtr(new MessageIterator(mq));
}

}  // namespace xcomet
