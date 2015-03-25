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
    const string& uid = (*msg)["to"].asString();
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
      LOG(ERROR) << "SaveMessageSync set max_seq failed" << s.code();
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
  VLOG(5) << "GetMessageSync uid = " << uid;
  MessageResult result(new vector<string>());
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
    result->reserve(keys.size() * 2);
    for (int i = last_ack + 1; i <= max_seq; ++i) {
      keys.push_back(IntToString(i));
    }
    LOG(INFO) << "keys.size=" << keys.size();
    s = client_->multi_hget(uid, keys, result.get());
    if (!s.ok()) {
      LOG(ERROR) << "GetMessageSync get keys failed: " << s.code();
    }
    LOG(INFO) << "result.size=" << result->size();
  }
  return result;
}

bool RemoteStorage::GetSeqs(const string& uid, int& min_seq, int& last_ack) {
  vector<string> keys(2);
  keys[0] = "min_seq";
  keys[1] = "last_ack";
  vector<string> results;
  results.reserve(keys.size() * 2);
  ssdb::Status s = client_->multi_hget(uid, keys, &results);
  if (s.ok()) {
    if (results.size() >= 4) {
      min_seq = StringToInt(results[1]);
      last_ack = StringToInt(results[3]);
    } else if (results.size() == 2 && results[0] == "last_ack") {
      // TODO(qingfeng) a little tricky, init it if not set
      // if min_seq not set, results will only contains last_ack
      last_ack = StringToInt(results[1]);
    } else {
      LOG(ERROR) << "GetSeqs invalid result size: " << results.size();
      return false;
    }
  } else {
    LOG(ERROR) << "GetSeqs failed: " << s.code();
    return false;
  }
  return true;
}

bool RemoteStorage::UpdateKey(const string& uid,
                              const string& key,
                              const string& value) {
  ssdb::Status s = client_->hset(uid, key, value);
  if (!s.ok()) {
    LOG(ERROR) << "UpdateKey " << key << " failed: " << s.code();
  }
  return s.ok();
}

bool RemoteStorage::DeleteMessageSync(const string uid) {
  ssdb::Status s;
  int min_seq = 0;
  int last_ack = 0;
  if (!GetSeqs(uid, min_seq, last_ack)) {
    return false;
  }
  VLOG(5) << "DeleteMessageSync: " << min_seq << ", " << last_ack;
  if (min_seq >= 0 && last_ack >=0 && last_ack > min_seq) {
    vector<string> keys;
    keys.reserve(last_ack - min_seq + 1);
    for (int i = min_seq; i <= last_ack; ++i) {
      keys.push_back(IntToString(i));
    }
    s = client_->multi_hdel(uid, keys);
    if (!s.ok()) {
      LOG(ERROR) << "DeleteMessageSync delete keys failed: " << s.code();
      return false;
    } else if (UpdateKey(uid, "min_seq", IntToString(last_ack))) {
      return true;
    }
  } else {
    LOG(ERROR) << "DeleteMessageSync invalid seqs";
  }
  return false;
}

int RemoteStorage::GetMaxSeqSync(const string uid) {
  string result;
  ssdb::Status s = client_->hget(uid, "max_seq", &result);
  if (!s.ok()) {
    LOG(ERROR) << "GetMaxSeqSync get max_seq failed: " << s.code();
    result = "0";
  }
  return StringToInt(result);
}

bool RemoteStorage::UpdateAckSync(const string uid, int ack_seq) {
  return UpdateKey(uid, "last_ack", IntToString(ack_seq));
}

}  // namespace xcomet
