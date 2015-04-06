#include "src/inmemory_storage.h"

#include "deps/base/time.h"
#include "src/loop_executor.h"

namespace xcomet {

const int INMEMORY_MSG_QUEUE_SIZE = 100;

InMemoryUserData::InMemoryUserData()
    : head_(0),
      head_seq_(0),
      tail_(0),
      tail_seq_(0),
      ack_(0),
      msg_queue_(INMEMORY_MSG_QUEUE_SIZE) {
}

InMemoryUserData::~InMemoryUserData() {
}

// current seconds
static int64 Now() {
  return base::GetTimeInSecond();
}

void InMemoryUserData::AddMessage(MessagePtr msg, int64 ttl) {
  CHECK(msg->isMember("seq"));
  int seq = (*msg)["seq"].asInt();
  VLOG(5) << "AddMessage seq=" << seq;
  CHECK(seq > 0);
  tail_seq_ = seq;
  if (ttl <= 0) {
    msg_queue_[tail_] = make_pair(0, Message::Serialize(msg));
  } else {
    int64 expired = Now() + ttl;
    msg_queue_[tail_] = make_pair(expired, Message::Serialize(msg));
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

MessageDataSet InMemoryUserData::GetMessages() {
  MessageDataSet result(NULL);
  int size = msg_queue_.size();
  if (size == 0) {
    return result;
  }
  int start_pos = head_seq_;
  if (ack_ > head_seq_) {
    start_pos = (head_ + ack_ - head_seq_ + size) % size;
  }
  int len = (tail_ + size - start_pos) % size;
  if (len > 0) {
    result.reset(new vector<string>());
    result->reserve(len);
    int64 now = Now();
    for (int i = start_pos; i < size && i < tail_; ++i) {
      if (now < msg_queue_[i].first) {
        result->push_back(*(msg_queue_[i].second));
      }
    }
    if (start_pos > tail_) {
      for (int i = 0; i < tail_; ++i) {
        if (now < msg_queue_[i].first) {
          result->push_back(*(msg_queue_[i].second));
        }
      }
    }
  }
  return result;
}

InMemoryStorage::InMemoryStorage() {
}

InMemoryStorage::~InMemoryStorage() {
}

void InMemoryStorage::SaveMessage(MessagePtr msg,
                                  int64 ttl,
                                  SaveMessageCallback cb) {
  const string& to = (*msg)["to"].asString();
  user_data_[to].AddMessage(msg, ttl);
  LoopExecutor::RunInMainLoop(bind(cb, NO_ERROR));
}

void InMemoryStorage::GetMessage(const string& uid, GetMessageCallback cb) {
  MessageDataSet msgs = user_data_[uid].GetMessages();
  LoopExecutor::RunInMainLoop(bind(cb, NO_ERROR, msgs));
}

void InMemoryStorage::GetMaxSeq(const string& uid, GetMaxSeqCallback cb) {
  int max_seq = user_data_[uid].GetMaxSeq();
  LoopExecutor::RunInMainLoop(bind(cb, NO_ERROR, max_seq));
}

void InMemoryStorage::UpdateAck(const string& uid,
                              int ack_seq,
                              UpdateAckCallback cb) {
  user_data_[uid].SetAck(ack_seq);
  LoopExecutor::RunInMainLoop(bind(cb, NO_ERROR));
}

void InMemoryStorage::AddUserToChannel(const string& uid,
                                       const string& cid,
                                       AddUserToChannelCallback cb) {
  channel_map_[cid].insert(uid);
  LoopExecutor::RunInMainLoop(bind(cb, NO_ERROR));
}

void InMemoryStorage::RemoveUserFromChannel(const string& uid,
                                            const string& cid,
                                            RemoveUserFromChannelCallback cb) {
  channel_map_[cid].erase(uid);
  LoopExecutor::RunInMainLoop(bind(cb, NO_ERROR));
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
  LoopExecutor::RunInMainLoop(bind(cb, NO_ERROR, users));
}

}  // namespace xcomet
