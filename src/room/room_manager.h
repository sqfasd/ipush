#ifndef SRC_ROOM_ROOMMANAGER_H_
#define SRC_ROOM_ROOMMANAGER_H_

#include "deps/jsoncpp/include/json/json.h"
#include "src/include_std.h"
#include "src/room/room.h"
#include "src/user.h"

namespace xcomet {
class SessionServer;
class RoomManager {
 public:
  RoomManager(SessionServer& server);
  void AddMember(const string& room_id, User* member);
  void AddMember(const string& room_id,
                 const string& member_id,
                 const int member_shard_id);
  void RemoveMember(const string& room_id, const string& member_id);
  void KickMember(const string& room_id, const string& member_id);
  void RoomBroadcast(const string& room_id, const string& body);
  void RoomSend(const string& room_id,
                const string& member_id,
                const string& body);
  void RoomSet(const string& room_id, const string& key, const string& value);
  void GetRoomState(const string& room_id, Json::Value& state);

 private:
  SessionServer& server_;
  unordered_map<string, Room> rooms_;
  typedef vector<string> RoomIdList;
  unordered_map<string, RoomIdList> user_rooms_;
};
}  // namespace xcomet
#endif  // SRC_ROOM_ROOMMANAGER_H_
