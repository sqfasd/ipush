#ifndef SRC_CHANNEL_H_
#define SRC_CHANNEL_H_

#include "deps/base/dlist.h"
#include "deps/base/logging.h"
#include "deps/base/shared_ptr.h"
#include "src/include_std.h"
#include "src/user.h"

namespace xcomet {

class Channel;
class SessionServer;
typedef base::shared_ptr<Channel> ChannelPtr;
typedef map<string, ChannelPtr> ChannelMap;

class Channel {
 public:
  Channel(const string& id) : id_(id) {
    CHECK(!id_.empty());
    VLOG(3) << "Channel construct: " << id_;
  }
  ~Channel() {
    VLOG(3) << "Channel destroy: " << id_;
  }
  string GetId() const {return id_;}
  int GetUserCount() const {return users_.Size();}
  void AddUser(User* user) {
    VLOG(3) << "channel AddUser: " << id_ << ", " << user->GetId();
    users_.PushBack(user);
    user->SetChannelId(id_);
    VLOG(3) << "channel size: " << users_.Size();
  }

  void RemoveUser(User* user) {
    VLOG(3) << "channel RemoveUser: " << id_ << ", " << user->GetId();
    users_.Remove(user);
    user->SetChannelId("-1");
  }
  void Broadcast(const string& content) {
    VLOG(3) << "channel size: " << users_.Size(); 
    DLinkedList<User*>::Iterator it = users_.GetIterator();
    while (User* user = it.Next()) {
      VLOG(3) << "channel Broadcast to: " << user->GetId();
      user->Send2(content);
    }
  }

 private:
  string id_;
  base::DLinkedList<User*> users_;
};
}  // namespace xcomet
#endif  // SRC_CHANNEL_H_
