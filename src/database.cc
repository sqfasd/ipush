#include "src/database.h"
#include "deps/base/logging.h"

DEFINE_string(db_host, "127.0.0.1", "ssdb");
DEFINE_int32(db_port, 8888, "ssdb");

namespace xcomet {

Database::Database(struct event_base* evbase) 
    : worker_(new Worker(evbase)) {
  option_.host = FLAGS_db_host;
  option_.port = FLAGS_db_port;
  client_.reset(ssdb::Client::connect(option_.host.c_str(), option_.port));
  CHECK(client_.get());
}

Database::Database(struct event_base* evbase, const DatabaseOption& option)
    : worker_(new Worker(evbase)), 
      option_(option) {
  client_.reset(ssdb::Client::connect(option_.host.c_str(), option_.port));
  CHECK(client_.get());
}

Database::~Database() {
}

void Database::SaveMessage(
    const string& uid,
    const string& content,
    boost::function<void (bool)> cb) {
  worker_->Do<bool>(boost::bind(&Database::SaveMessageSync, this, uid, content), cb);
}

bool Database::SaveMessageSync(const string uid, const string content) {
  ssdb::Status s;
  s = client_->qpush(uid, content);
  return s.ok();
}

void Database::GetMessageIterator(
    const string& uid,
    boost::function<void (MessageIteratorPtr)> cb) {
  worker_->Do<MessageIteratorPtr>(boost::bind(&Database::GetMessageIteratorSync, this, uid), cb);
}

MessageIteratorPtr Database::GetMessageIteratorSync(const string uid) {
  vector<string> result;
  // TODO avoid inefficient copy
  client_->qslice(uid, 0, -1, &result);
  base::shared_ptr<queue<string> > mq(new queue<string>());
  for (size_t i = 0; i < result.size(); ++i) {
    mq->push(result[i]);
  }
  return MessageIteratorPtr(new MessageIterator(mq));
}

void Database::GetMessageIterator(
    const string& uid,
    int64_t start,
    int64_t end,
    boost::function<void (MessageIteratorPtr)> cb) {
  worker_->Do<MessageIteratorPtr>(boost::bind(&Database::GetMessageIteratorSync, this, uid, start, end), cb);
}

MessageIteratorPtr Database::GetMessageIteratorSync(const string uid, int64_t start, int64_t end) {
  vector<string> result;
  base::shared_ptr<queue<string> > mq(new queue<string>());
  ssdb::Status s = client_->qslice(uid, start, end, &result);
  if(!s.ok()) {
    LOG(ERROR) << s.code();
    return MessageIteratorPtr(new MessageIterator(mq));
  }
  VLOG(5) << "qslice " << start << "," << end;
  for(size_t i = 0; i < result.size(); ++i ) {
    mq->push(result[i]);
  }
  return MessageIteratorPtr(new MessageIterator(mq));
}

}  // namespace xcomet
