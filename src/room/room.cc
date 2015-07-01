#include "src/room/room.h"
#include "src/message.h"

namespace xcomet {

Room::Room(const string& id) : id_(id) {
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
  for (auto& i : members_) {
    i.second->Send(msg);
  }
}

void Room::RemoveMember(const string& member_id) {
  VLOG(3) << "Room RemoveMember: " << member_id;
  auto iter = members_.find(member_id);
  if (iter == members_.end()) {
    VLOG(3) << "member not found: " << member_id << ", in the room: " << id_;
    return;
  }
  iter->second->LeaveRoom(id_);
  members_.erase(iter);
  Message msg;
  msg.SetType(Message::T_ROOM_LEAVE);
  msg.SetRoom(id_);
  msg.SetUser(member_id);
  for (auto& i : members_) {
    i.second->Send(msg);
  }
}

void Room::Broadcast(const string& body) {
  VLOG(3) << "Room Broadcast: " << body;
  Message msg;
  msg.SetType(Message::T_ROOM_BROADCAST);
  msg.SetBody(body);
  for (auto& i : members_) {
    i.second->Send(msg);
  }
}

void Room::Send(const string& member_id, const string& body) {
  VLOG(3) << "Room Send: " << member_id << ", " << body;
  auto iter = members_.find(member_id);
  if (iter == members_.end()) {
    VLOG(3) << "member not found: " << member_id;
    return;
  }
  Message msg;
  msg.SetType(Message::T_ROOM_SEND);
  msg.SetBody(body);
  iter->second->Send(msg);
}

}  // namespace xcomet
