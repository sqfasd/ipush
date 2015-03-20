#include "deps/jsoncpp/include/json/json.h"
#include "deps/base/logging.h"
#include "deps/base/string_util.h"
#include "src/remote_storage.h"

DEFINE_string(db_host, "127.0.0.1", "ssdb");
DEFINE_int32(db_port, 8888, "ssdb");

namespace xcomet {

RemoteStorage::RemoteStorage(struct event_base* evbase)
    : Storage(evbase) {
  option_.host = FLAGS_db_host;
  option_.port = FLAGS_db_port;
  client_.reset(ssdb::Client::connect(option_.host.c_str(), option_.port));
  CHECK(client_.get());
}

RemoteStorage::RemoteStorage(struct event_base* evbase,
                             const RemoteStorageOption& option)
    : Storage(evbase), 
      option_(option) {
  client_.reset(ssdb::Client::connect(option_.host.c_str(), option_.port));
  CHECK(client_.get());
}

RemoteStorage::~RemoteStorage() {
}

bool RemoteStorage::SaveMessageSync(MessagePtr msg, int seq) {
  try {
    ssdb::Status s;
    const string& uid = (*msg)["from"].asString();
    if (seq <= 0) {
      string result;
      s = client_->hget(uid, "max_seq", &result);
      if (!s.ok()) {
        result = "0";
      }
      seq = StringToInt(result) + 1;
    }
    VLOG(5) << "set max_seq: " << seq;
    (*msg)["seq"] = seq;
    s = client_->hset(uid, "max_seq", IntToString(seq));
    if (!s.ok()) {
      LOG(ERROR) << "SaveMessageSync set max_seq failed";
      return false;
    }
    Json::FastWriter writer;
    string content = writer.write(*msg);
    s = client_->hset(uid, IntToString(seq), content);
    if (!s.ok()) {
      LOG(ERROR) << "SaveMessageSync set msg failed";
      return false;
    }
    return true;
  } catch (std::exception& e) {
    LOG(ERROR) << "SaveMessageSync json exception: " << e.what();
    return false;
  } catch (...) {
    LOG(ERROR) << "unknow exception";
    return false;
  }
}

MessageResult RemoteStorage::GetMessageSync(const string uid) {
  MessageResult result(new vector<string>());
  result->reserve(DEFAULT_BATCH_GET_SIZE);
  ssdb::Status s;
  int max_seq = 0;
  int last_ack = 0;
  string tmp;
  s = client_->hget(uid, "max_seq", &tmp);
  if (s.ok()) {
    max_seq = StringToInt(tmp);
  }
  VLOG(5) << "max_seq = " << max_seq;
  tmp.clear();
  s = client_->hget(uid, "last_ack", &tmp);
  if (s.ok()) {
    last_ack = StringToInt(tmp);
  }
  VLOG(5) << "last_ack = " << last_ack;
  if (max_seq > 0 && last_ack >= 0 && max_seq > last_ack) {
    vector<string> keys;
    keys.reserve(max_seq - last_ack);
    for (int i = last_ack + 1; i <= max_seq; ++i) {
      keys.push_back(IntToString(i));
    }
    LOG(INFO) << "keys.size=" << keys.size();
    s = client_->multi_hget(uid, keys, result.get());
    if (!s.ok()) {
      LOG(ERROR) << "GetMessageSync get keys failed";
    }
    LOG(INFO) << "result.size=" << result->size();
    for (int i = 0; i < result->size(); ++i) {
      LOG(INFO) << "result=" << result->at(i);
    }
  }
  return result;
}

MessageResult RemoteStorage::GetMessageSync(
    const string uid,
    int64_t start,
    int64_t end) {
  MessageResult result(new vector<string>());
  result->reserve(DEFAULT_BATCH_GET_SIZE);
  ssdb::Status s = client_->qslice(uid, start, end, result.get());
  if(!s.ok()) {
    LOG(ERROR) << "GetMessageSync failed: " << s.code();
  }
  return result;
}

bool RemoteStorage::UpdateAckSync(const string uid, int ack_seq) {
  ssdb::Status s;
  int max_seq = 0;
  int last_ack = 0;
  string tmp;
  s = client_->hget(uid, "max_seq", &tmp);
  if (s.ok()) {
    max_seq = StringToInt(tmp);
  }
  tmp.clear();
  s = client_->hget(uid, "last_ack", &tmp);
  if (s.ok()) {
    last_ack = StringToInt(tmp);
  }
  if (max_seq >= 0 && last_ack >= 0 &&
      ack_seq > last_ack && ack_seq <= max_seq) {
    vector<string> keys;
    keys.reserve(ack_seq - last_ack + 1);
    for (int i = last_ack; i <= ack_seq; ++i) {
      keys.push_back(IntToString(i));
    }
    s = client_->multi_hdel(uid, keys);
    if (!s.ok()) {
      LOG(ERROR) << "UpdateAckSync delete keys failed";
    } else {
      return true;
    }
  } else {
    LOG(ERROR) << "UpdateAckSync get seqs failed: "
               << last_ack << ", " << max_seq;
  }
  return false;
}

int RemoteStorage::GetMaxSeqSync(const string uid) {
  string result;
  ssdb::Status s = client_->hget(uid, "max_seq", &result);
  if (!s.ok()) {
    LOG(ERROR) << "GetMaxSeqSync get last_ack failed";
    result = "0";
  }
  return StringToInt(result);
}

}  // namespace xcomet
