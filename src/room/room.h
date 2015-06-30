#ifndef SRC_ROOM_ROOM_H_
#define SRC_ROOM_ROOM_H_

#include "src/include_std.h"
#include "src/user.h"

namespace xcomet {
class Room {
 public:
  Room(const string& id);
  void AddMember(User* member);
  void RemoveMember(const string& member_id);
  void Broadcast(const string& body);
  void Send(const string& member_id, const string& body);

 private:
  unordered_map<string, User*> members_;
  string id_;
};
}  // namespace xcomet
#endif  // SRC_ROOM_ROOM_H_
