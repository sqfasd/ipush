#include "src/room/room.h"
#include "src/message.h"
#include "src/session_server.h"

namespace xcomet {

Room::Room(const string& id, SessionServer& server) : id_(id), server_(server) {
  VLOG(3) << "Room construct";
}

void Room::AddMember(User* member) {
  VLOG(3) << "Room AddMember: " << member->GetId();
  members_[member->GetId()] = member;
  member->JoinRoom(id_);
  Message msg;
  msg.SetType(Message::T_ROOM_JOIN);
  msg.SetRoom(id_);
  msg.SetUser(member->GetId());
  NotifyAll(msg);
}

void Room::AddMember(const string& member_id, const int member_shard_id) {
  VLOG(3) << "Room AddMember: " << member_id
          << ", from shard: " << member_shard_id;
  shard_members_[member_id] = member_shard_id;
  Message msg;
  msg.SetType(Message::T_ROOM_JOIN);
  msg.SetRoom(id_);
  msg.SetUser(member_id);
  NotifyAll(msg);
}

void Room::RemoveMember(const string& member_id) {
  VLOG(3) << "Room RemoveMember: " << member_id;
  auto iter = members_.find(member_id);
  if (iter == members_.end()) {
    VLOG(3) << "member not found: " << member_id << ", in the room: " << id_;
    return;
  }
  members_.erase(iter);
  Message msg;
  msg.SetType(Message::T_ROOM_LEAVE);
  msg.SetRoom(id_);
  msg.SetUser(member_id);
  NotifyAll(msg);
}

void Room::Broadcast(const string& body) {
  VLOG(3) << "Room Broadcast: " << body;
  Message msg;
  msg.SetType(Message::T_ROOM_BROADCAST);
  msg.SetBody(body);
  NotifyAll(msg);
}

void Room::Send(const string& member_id, const string& body) {
  VLOG(3) << "Room Send: " << member_id << ", " << body;
  Message msg;
  msg.SetType(Message::T_ROOM_SEND);
  msg.SetBody(body);
  auto iter = members_.find(member_id);
  if (iter != members_.end()) {
    iter->second->Send(msg);
  } else {
    auto iter_shard_member = shard_members_.find(member_id);
    if (iter_shard_member != shard_members_.end()) {
      server_.RedirectUserMessage(iter_shard_member->second, member_id, msg);
    } else {
      VLOG(3) << "target user not found: " << member_id;
    }
  }
}

void Room::Set(const string& key, const string& value) {
  VLOG(3) << "Room Set: " << key << ", " << value;
  attrs_[key] = value;
  Message msg;
  msg.SetType(Message::T_ROOM_SET);
  msg.SetKey(key);
  msg.SetValue(value);
  NotifyAll(msg);
}

void Room::NotifyAll(const Message& msg) {
  for (auto& i : members_) {
    i.second->Send(msg);
  }
  for (auto& i : shard_members_) {
    const string& member_id = i.first;
    int shard_id = i.second;
    server_.RedirectUserMessage(shard_id, member_id, msg);
  }
}

}  // namespace xcomet
