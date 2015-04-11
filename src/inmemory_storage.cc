#include "src/inmemory_storage.h"

#include "deps/base/time.h"
#include "deps/base/file.h"
#include "deps/base/string_util.h"
#include "deps/base/flags.h"
#include "src/loop_executor.h"

using base::File;
using base::Time;

namespace xcomet {

DEFINE_int32(max_offline_msg_num, 100, "");
DEFINE_string(inmemory_data_dir, "inmemory_data", "");

// current seconds
static int64 Now() {
  return base::GetTimeInSecond();
}

InMemoryUserData::InMemoryUserData()
    : head_(0),
      head_seq_(0),
      tail_(0),
      tail_seq_(0),
      ack_(0) {
  CHECK(FLAGS_max_offline_msg_num > 0);
  msg_queue_.resize(FLAGS_max_offline_msg_num);
}

InMemoryUserData::~InMemoryUserData() {
}

bool InMemoryUserData::Dump(Json::Value& json) {
  if (tail_seq_ == 0) {
    return false;
  }
  json["head"] = head_;
  json["head_seq"] = head_seq_;
  json["tail"] = tail_;
  json["tail_seq"] = tail_seq_;
  json["ack"] = ack_;
  json["msgs"] = Json::Value(Json::arrayValue);
  Json::Value& msgs = json["msgs"];

  int start_pos;
  int len;
  GetQueueInfo(start_pos, len);
  if (len <= 0) {
    return true;
  }
  int size = msg_queue_.size();
  int64 now = Now();
  for (int i = start_pos; i < size && i < tail_; ++i) {
    if (IsMsgOK(now, msg_queue_[i])) {
      Json::Value msg;
      msg["i"] = i;
      msg["t"] = static_cast<Json::Int64>(msg_queue_[i].first);
      msg["b"] = *(msg_queue_[i].second);
      msgs.append(msg);
    }
  }
  if (start_pos > tail_) {
    for (int i = 0; i < tail_; ++i) {
      if (IsMsgOK(now, msg_queue_[i])) {
        Json::Value msg;
        msg["i"] = i;
        msg["t"] = static_cast<Json::Int64>(msg_queue_[i].first);
        msg["b"] = *(msg_queue_[i].second);
        msgs.append(msg);
      }
    }
  }
  return true;
}

void InMemoryUserData::Load(const Json::Value& json) {
  CHECK(json.isMember("head"));
  CHECK(json.isMember("head_seq"));
  CHECK(json.isMember("tail"));
  CHECK(json.isMember("tail_seq"));
  CHECK(json.isMember("ack"));
  CHECK(json.isMember("msgs"));
  head_ = json["head"].asInt();
  head_seq_ = json["head_seq"].asInt();
  tail_ = json["tail"].asInt();
  tail_seq_ = json["tail_seq"].asInt();
  ack_ = json["ack"].asInt();
  for (Json::ArrayIndex i = 0; i < json["msgs"].size(); ++i) {
    const Json::Value& msg = json["msgs"][i];
    CHECK(msg.isMember("i"));
    CHECK(msg.isMember("t"));
    CHECK(msg.isMember("b"));
    int64 expired_time = msg["t"].asInt64();
    int index = msg["i"].asInt();
    StringPtr data(new string());
    *data = msg["b"].asString();
    msg_queue_[index] = make_pair(expired_time, data);
  }
}

void InMemoryUserData::AddMessage(const StringPtr& msg, int64 ttl) {
  ++tail_seq_;
  VLOG(6) << "tail_seq=" << tail_seq_ << ", tail=" << tail_;
  if (ttl <= 0) {
    msg_queue_[tail_] = make_pair(0, msg);
  } else {
    int64 expired = Now() + ttl;
    msg_queue_[tail_] = make_pair(expired, msg);
  }
  if (++tail_ == msg_queue_.size()) {
    tail_ = 0;
  }
  if (tail_ == head_) {
    ++head_seq_;
    if (++head_ == msg_queue_.size()) {
      head_ = 0;
    }
  }
}

void InMemoryUserData::GetQueueInfo(int& start_pos, int& len) {
  int size = msg_queue_.size();
  CHECK(size > 0);
  start_pos = head_seq_;
  VLOG(6) << "start_pos=" << start_pos;
  if (ack_ > head_seq_) {
    start_pos = (head_ + ack_ - head_seq_ + size) % size;
  }
  VLOG(6) << "update start_pos=" << start_pos;
  len = (tail_ + size - start_pos) % size;
  VLOG(6) << "tail=" << tail_ << ", msg queue len=" << len;
}

bool InMemoryUserData::IsMsgOK(int64 now, const pair<int64, StringPtr>& msg) {
  return (msg.first <= 0 || now < msg.first) && msg.second.get() != NULL;
}

MessageDataSet InMemoryUserData::GetMessages() {
  MessageDataSet result(NULL);
  int start_pos;
  int len;
  GetQueueInfo(start_pos, len);
  if (len > 0) {
    result.reset(new vector<string>());
    result->reserve(len);
    int size = msg_queue_.size();
    int64 now = Now();
    for (int i = start_pos; i < size && i < tail_; ++i) {
      if (IsMsgOK(now, msg_queue_[i])) {
        result->push_back(*(msg_queue_[i].second));
      }
    }
    if (start_pos > tail_) {
      for (int i = 0; i < tail_; ++i) {
        if (IsMsgOK(now, msg_queue_[i])) {
          result->push_back(*(msg_queue_[i].second));
        }
      }
    }
  }
  return result;
}

InMemoryStorage::InMemoryStorage() {
  Load();
}

InMemoryStorage::~InMemoryStorage() {
  Dump();
}

static string CurrentTime() {
  Time::Exploded time;
  Time::Now().LocalExplode(&time);
  return StringPrintf("%04d%02d%02d%02d%02d%02d",
      time.year,
      time.month,
      time.day_of_month,
      time.hour,
      time.minute,
      time.second);

}

void InMemoryStorage::Dump() {
  LOG(INFO) << "InMemoryStorage::Dump ...";
  string dir = File::JoinPath(FLAGS_inmemory_data_dir, CurrentTime());
  if (!File::IsDir(dir)) {
    File::RecursivelyCreateDir(dir, 0777);
  }

  Json::FastWriter writer;
  string user_file = File::JoinPath(dir, "user");
  for (auto& i : user_data_) {
    Json::Value v;
    v["name"] = i.first;
    if (i.second.Dump(v["imud"])) {
      File::AppendStringToFile(writer.write(v), user_file);
    }
  }

  string channel_file = File::JoinPath(dir, "channel");
  for (auto& i : channel_map_) {
    Json::Value channel;
    channel["name"] = i.first;
    channel["users"] = Json::Value(Json::arrayValue);
    Json::Value& users = channel["users"];
    for (auto& j : i.second) {
      users.append(j);
    }
    File::AppendStringToFile(writer.write(channel), channel_file);
  }
}

void InMemoryStorage::Load() {
  LOG(INFO) << "InMemoryStorage::Load ...";
  if (!File::IsDir(FLAGS_inmemory_data_dir)) {
    LOG(INFO) << "no inmemory dump data";
    return;
  }
  vector<string> data_dirs;
  CHECK(base::File::GetDirsInDir(FLAGS_inmemory_data_dir, &data_dirs));
  LOG(INFO) << data_dirs.size() << " inmemory data dirs";
  if (data_dirs.empty()) {
    return;
  }
  std::sort(data_dirs.begin(), data_dirs.end());
  const string latest_dir = data_dirs[data_dirs.size() - 1];
  LOG(INFO) << "select the latest inmemory dump data: " << latest_dir;
  Json::Reader parser;
  ifstream reader;
  string line;

  string user_file = File::JoinPath(latest_dir, "user");
  if (File::Exists(user_file)) {
    reader.open(user_file.c_str(), ifstream::in);
    CHECK(reader.is_open());
    while (getline(reader, line)) {
      Json::Value json;
      CHECK(parser.parse(line, json));
      CHECK(json.isMember("name"));
      CHECK(json.isMember("imud"));
      const string& name = json["name"].asString();
      auto it = user_data_.insert(make_pair(name, InMemoryUserData()));
      it.first->second.Load(json["imud"]);
    }
    reader.close();
  }

  string channel_file = File::JoinPath(latest_dir, "channel");
  if (File::Exists(channel_file)) {
    reader.open(channel_file.c_str(), ifstream::in);
    CHECK(reader.is_open());
    while (getline(reader, line)) {
      Json::Value json;
      CHECK(parser.parse(line, json));
      CHECK(json.isMember("name"));
      CHECK(json.isMember("users"));
      const string& name = json["name"].asString();
      auto it = channel_map_.insert(make_pair(name, unordered_set<string>()));
      const Json::Value& users = json["users"];
      CHECK(users.type() == Json::arrayValue);
      for (Json::ArrayIndex i = 0; i < users.size(); ++i) {
        it.first->second.insert(users[i].asString());
      }
    }
  }
}

static void Callback(function<void()> f) {
  // LoopExecutor::RunInMainLoop(f);
  f();
}

void InMemoryStorage::SaveMessage(const StringPtr& msg,
                                  const string& uid,
                                  int64 ttl,
                                  SaveMessageCallback cb) {
  VLOG(6) << "SaveMessage " << uid << ": " << *msg;
  user_data_[uid].AddMessage(msg, ttl);
  Callback(bind(cb, NO_ERROR));
}

void InMemoryStorage::GetMessage(const string& uid, GetMessageCallback cb) {
  MessageDataSet msgs = user_data_[uid].GetMessages();
  Callback(bind(cb, NO_ERROR, msgs));
}

void InMemoryStorage::GetMaxSeq(const string& uid, GetMaxSeqCallback cb) {
  int max_seq = user_data_[uid].GetMaxSeq();
  Callback(bind(cb, NO_ERROR, max_seq));
}

void InMemoryStorage::UpdateAck(const string& uid,
                              int ack_seq,
                              UpdateAckCallback cb) {
  user_data_[uid].SetAck(ack_seq);
  Callback(bind(cb, NO_ERROR));
}

void InMemoryStorage::AddUserToChannel(const string& uid,
                                       const string& cid,
                                       AddUserToChannelCallback cb) {
  channel_map_[cid].insert(uid);
  Callback(bind(cb, NO_ERROR));
}

void InMemoryStorage::RemoveUserFromChannel(const string& uid,
                                            const string& cid,
                                            RemoveUserFromChannelCallback cb) {
  channel_map_[cid].erase(uid);
  Callback(bind(cb, NO_ERROR));
}

void InMemoryStorage::GetChannelUsers(const string& cid,
                                      GetChannelUsersCallback cb) {
  UserResultSet users(NULL);
  auto iter = channel_map_.find(cid);
  if (iter != channel_map_.end()) {
    users.reset(new vector<string>());
    users->reserve(iter->second.size());
    users->assign(iter->second.begin(), iter->second.end());
  }
  Callback(bind(cb, NO_ERROR, users));
}

}  // namespace xcomet
