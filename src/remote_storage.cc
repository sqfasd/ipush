#include "src/remote_storage.h"
#include "deps/base/logging.h"

DEFINE_string(db_host, "127.0.0.1", "ssdb");
DEFINE_int32(db_port, 8888, "ssdb");

namespace xcomet {

RemoteStorage::RemoteStorage(struct event_base* evbase) 
    : worker_(new Worker(evbase)) {
  option_.host = FLAGS_db_host;
  option_.port = FLAGS_db_port;
  client_.reset(ssdb::Client::connect(option_.host.c_str(), option_.port));
  CHECK(client_.get());
}

RemoteStorage::RemoteStorage(struct event_base* evbase, const RemoteStorageOption& option)
    : worker_(new Worker(evbase)), 
      option_(option) {
  client_.reset(ssdb::Client::connect(option_.host.c_str(), option_.port));
  CHECK(client_.get());
}

RemoteStorage::~RemoteStorage() {
}

void RemoteStorage::SaveMessage(
    const string& uid,
    const string& content,
    boost::function<void (bool)> cb) {
  worker_->Do<bool>(boost::bind(&RemoteStorage::SaveMessageSync, this, uid, content), cb);
}

bool RemoteStorage::SaveMessageSync(const string uid, const string content) {
  ssdb::Status s;
  s = client_->qpush(uid, content);
  return s.ok();
}

void RemoteStorage::PopMessageIterator(
    const string& uid,
    boost::function<void (MessageIteratorPtr)> cb) {
  worker_->Do<MessageIteratorPtr>(boost::bind(&RemoteStorage::PopMessageIteratorSync, this, uid), cb);
}

MessageIteratorPtr RemoteStorage::PopMessageIteratorSync(const string uid) {
  string str1;
  base::shared_ptr<queue<string> > mq(new queue<string>());
  ssdb::Status s;
  while(true){
    s = client_->qpop(uid, &str1);
    if(!s.ok()){
      break;
    }
    mq->push(str1);
  }
  return MessageIteratorPtr(new MessageIterator(mq));
}

void RemoteStorage::GetMessageIterator(
    const string& uid,
    boost::function<void (MessageIteratorPtr)> cb) {
  worker_->Do<MessageIteratorPtr>(boost::bind(&RemoteStorage::GetMessageIteratorSync, this, uid), cb);
}

MessageIteratorPtr RemoteStorage::GetMessageIteratorSync(const string uid) {
  vector<string> result;
  // TODO avoid inefficient copy
  client_->qslice(uid, 0, -1, &result);
  base::shared_ptr<queue<string> > mq(new queue<string>());
  for (size_t i = 0; i < result.size(); ++i) {
    mq->push(result[i]);
  }
  return MessageIteratorPtr(new MessageIterator(mq));
}

void RemoteStorage::GetMessageIterator(
    const string& uid,
    int64_t start,
    int64_t end,
    boost::function<void (MessageIteratorPtr)> cb) {
  worker_->Do<MessageIteratorPtr>(boost::bind(&RemoteStorage::GetMessageIteratorSync, this, uid, start, end), cb);
}

MessageIteratorPtr RemoteStorage::GetMessageIteratorSync(const string uid, int64_t start, int64_t end) {
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
