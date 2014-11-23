#ifndef SRC_USER_H_
#define SRC_USER_H_

#include <evhttp.h>
#include "base/dlist.h"
#include "base/shared_ptr.h"
#include "src/include_std.h"

using base::DLinkedList;

namespace xcomet {

class SessionServer;
class User;
class UserCircleQueue;

typedef base::shared_ptr<User> UserPtr;
class User {
 public:
   enum {
     COMET_TYPE_STREAM = 1,
     COMET_TYPE_POLLING,
   };

  User(const string& uid, int type, struct evhttp_request* req, SessionServer& serv);
  ~User();
  void SetType(int type) {type_ = type;}
  int GetType() {return type_;}
  string GetUid() {return uid_;}
  void Send(const string& content);
  void SendHeartbeat();
  void Close();

 private:
  static void OnDisconnect(struct evhttp_connection* evconn, void* arg);
  void SendChunk(const string& type, const string& content);

  User* prev_;
  User* next_;
  int queue_index_;
  string uid_;
  int type_;
  struct evhttp_request* req_;
  SessionServer& server_;

  friend class DLinkedList<User*>;
  friend class UserCircleQueue;
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
