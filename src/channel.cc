#include "src/channel.h"
#include "deps/base/logging.h"

namespace xcomet {

Channel::Channel(const string& id) : id_(id) {
  CHECK(!id_.empty());
  VLOG(3) << "Channel construct: " << id_;
}

Channel::~Channel() {
  VLOG(3) << "Channel destroy: " << id_;
}

void Channel::AddUser(User* user) {
  VLOG(3) << "channel AddUser: " << id_ << ", " << user->GetId();
  users_.insert(make_pair(user->GetId(), user));
  user->SetChannelId(id_);
}

void Channel::RemoveUser(User* user) {
  VLOG(3) << "channel RemoveUser: " << id_ << ", " << user->GetId();
  users_.erase(user->GetId());
  user->SetChannelId("-1");
}

void Channel::Broadcast(const string& content) {
  map<string, User*>::iterator it = users_.begin();
  while (it != users_.end()) {
    VLOG(3) << "channel Broadcast to: " << it->second->GetId();
    it->second->Send2(content);
    ++it;
  }
}

}  // namespace xcomet
