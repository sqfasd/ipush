#ifndef SRC_USER_H_
#define SRC_USER_H_

#include <boost/bind.hpp>
#include "deps/base/dlist.h"
#include "deps/base/shared_ptr.h"
#include "src/include_std.h"
#include "src/session.h"

using base::DLinkedList;

namespace xcomet {

class User;
class UserCircleQueue;
class SessionServer;
typedef base::shared_ptr<User> UserPtr;
typedef map<string, UserPtr> UserMap;

class User {
 public:
   enum {
     COMET_TYPE_STREAM = 1,
     COMET_TYPE_POLLING,
   };

  User(const string& uid, int type, struct evhttp_request* req, SessionServer& serv);
  ~User();
  void SetType(int type) {type_ = type;}
  int GetType() const {return type_;}
  string GetId() const {return uid_;}
  void Send(const string& content);
  void Send2(const string& content);
  void Close();
  void SendHeartbeat();
  string GetChannelId() const {return channel_id_;}
  void SetChannelId(const string& cid) {channel_id_ = cid;}

 private:
  void OnSessionDisconnected();
  Session session_;
  User* prev_;
  User* next_;
  int queue_index_;
  string uid_;
  int type_;
  string channel_id_;

  SessionServer& server_;

  friend class DLinkedList<User*>;
  friend class UserCircleQueue;
  friend class Channel;
};

class UserCircleQueue {
 public:
  UserCircleQueue(int max_size) : size_(max_size), head_(0) {
    buffer_.resize(size_);
  }
  
  DLinkedList<User*> PopFront() {
    DLinkedList<User*> ret = buffer_[head_];
    buffer_[head_] = DLinkedList<User*>();
    return ret;
  }

  void RemoveUser(User* user) {
    if (user->queue_index_ != -1) {
      buffer_[user->queue_index_].Remove(user);
    }
  }

  void PushUserBack(User* user) {
    RemoveUser(user);
    int tail = (head_ - 1 + size_) % size_;
    buffer_[tail].PushBack(user);
    user->queue_index_ = tail;
  }

  void IncHead() {
    head_ = (head_ + 1) % size_;
  }

 private:
  const int size_;
  int head_;
  vector<DLinkedList<User*> > buffer_;
};

}  // namespace xcomet
#endif  // SRC_USER_H_
