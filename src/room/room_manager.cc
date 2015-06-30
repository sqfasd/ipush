#include "src/room/room_manager.h"

namespace xcomet {

RoomManager::RoomManager() {
}

void RoomManager::AddMember(const string& room_id, User* member) {
  if (member == NULL) {
    VLOG(3) << "add NULL member to room: " << room_id;
    return;
  }
  auto iter = rooms_.find(room_id);
  if (iter == rooms_.end()) {
    pair<unordered_map<string, Room>::iterator, bool> ret;
    ret = rooms_.insert(make_pair(room_id, Room(room_id)));
    if (!ret.second) {
      LOG(ERROR) << "add new room failed, room id: " << room_id;
      return;
    }
    iter = ret.first;
  }
  iter->second.AddMember(member);
}

void RoomManager::RemoveMember(const string& room_id, const string& member_id) {
  auto iter = rooms_.find(room_id);
  if (iter == rooms_.end()) {
    VLOG(3) << " room not found: " << room_id;
    return;
  }
  iter->second.RemoveMember(member_id);
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

}  // namespace xcomet
