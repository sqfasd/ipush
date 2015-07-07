#include "src/room/room_manager.h"
#include "src/session_server.h"

namespace xcomet {

RoomManager::RoomManager(SessionServer& server) : server_(server) {
}

void RoomManager::AddMember(const string& room_id, User* member) {
  if (member == NULL) {
    VLOG(3) << "add NULL member to room: " << room_id;
    return;
  }
  auto iter = rooms_.find(room_id);
  if (iter == rooms_.end()) {
    pair<unordered_map<string, Room>::iterator, bool> ret;
    ret = rooms_.insert(make_pair(room_id, Room(room_id, server_)));
    if (!ret.second) {
      LOG(ERROR) << "add new room failed, room id: " << room_id;
      return;
    }
    iter = ret.first;
  }
  iter->second.AddMember(member);
}

void RoomManager::AddMember(const string& room_id,
                            const string& member_id,
                            const int member_shard_id) {
  auto iter = rooms_.find(room_id);
  if (iter == rooms_.end()) {
    pair<unordered_map<string, Room>::iterator, bool> ret;
    ret = rooms_.insert(make_pair(room_id, Room(room_id, server_)));
    if (!ret.second) {
      LOG(ERROR) << "add new room failed, room id: " << room_id;
      return;
    }
    iter = ret.first;
  }
  iter->second.AddMember(member_id, member_shard_id);
}

void RoomManager::RemoveMember(const string& room_id, const string& member_id) {
  auto iter = rooms_.find(room_id);
  if (iter == rooms_.end()) {
    VLOG(3) << " room not found: " << room_id;
    return;
  }
  iter->second.RemoveMember(member_id);
}

void RoomManager::KickMember(const string& room_id, const string& member_id) {
  RemoveMember(room_id, member_id);
}

void RoomManager::RoomBroadcast(const string& room_id, const string& body) {
  auto iter = rooms_.find(room_id);
  if (iter == rooms_.end()) {
    VLOG(3) << " room not found: " << room_id;
    return;
  }
  iter->second.Broadcast(body);
}

void RoomManager::RoomSend(const string& room_id,
                           const string& member_id,
                           const string& body) {
  auto iter = rooms_.find(room_id);
  if (iter == rooms_.end()) {
    VLOG(3) << " room not found: " << room_id;
    return;
  }
  iter->second.Send(member_id, body);
}

void RoomManager::RoomSet(const string& room_id,
                          const string& key,
                          const string& value) {
  auto iter = rooms_.find(room_id);
  if (iter == rooms_.end()) {
    VLOG(3) << " room not found: " << room_id;
    return;
  }
  iter->second.Set(key, value);
}

void RoomManager::GetRoomState(const string& room_id, Json::Value& state) {
  auto iter = rooms_.find(room_id);
  if (iter == rooms_.end()) {
    VLOG(3) << " room not found: " << room_id;
    return;
  }
  Json::Value& members = state["members"];
  Json::Value& attrs = state["attrs"];
  for (auto& i : iter->second.members()) {
    members.append(i.second->GetId());
  }
  for (auto& i : iter->second.shard_members()) {
    members.append(i.first);
  }
  for (auto& i : iter->second.attrs()) {
    Json::Value item;
    item[i.first] = i.second;
    attrs.append(item);
  }
}

}  // namespace xcomet
