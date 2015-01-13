#include "src/channel.h"

#include "deps/jsoncpp/include/json/json.h"
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
  user->SubChannel(id_);
}

void Channel::RemoveUser(User* user) {
  VLOG(3) << "channel RemoveUser: " << id_ << ", " << user->GetId();
  users_.erase(user->GetId());
  user->UnsubChannel(id_);
}

void Channel::Broadcast(const string& packet_str) const {
  map<string, User*>::const_iterator it = users_.begin();
  while (it != users_.end()) {
    VLOG(3) << "channel Broadcast to: " << it->second->GetId();
    it->second->SendPacket(packet_str);
    ++it;
  }
}

void Channel::Broadcast(const string& from, const string& content) const {
  Json::Value json;
  json["type"] = "channel";
  json["from"] = from;
  json["to"] = id_;
  json["content"] = content;
  Json::FastWriter writer;
  string packet_str = writer.write(json);
  map<string, User*>::const_iterator it = users_.begin();
  while (it != users_.end()) {
    VLOG(3) << "channel Broadcast to: " << it->second->GetId();
    it->second->SendPacket(packet_str);
    ++it;
  }
}

}  // namespace xcomet
