#ifndef SRC_ROOM_ROOM_H_
#define SRC_ROOM_ROOM_H_

#include "src/include_std.h"
#include "src/user.h"

namespace xcomet {
class SessionServer;
class Message;
class Room {
 public:
  Room(const string& id, SessionServer& server);
  void AddMember(User* member);
  void AddMember(const string& member_id, const int member_shard_id);
  void RemoveMember(const string& member_id);
  void Broadcast(const string& body);
  void Send(const string& member_id, const string& body);
  void Set(const string& key, const string& value);
  const unordered_map<string, User*> members() const {
    return members_;
  }
  const unordered_map<string, int> shard_members() const {
    return shard_members_;
  }
  const unordered_map<string, string> attrs() const {
    return attrs_;
  }

 private:
  void NotifyAll(const Message& msg);

  string id_;
  SessionServer& server_;
  unordered_map<string, User*> members_;
  unordered_map<string, int> shard_members_;
  unordered_map<string, string> attrs_;
};
}  // namespace xcomet
#endif  // SRC_ROOM_ROOM_H_
