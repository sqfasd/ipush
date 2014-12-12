#ifndef SRC_CHANNEL_H_
#define SRC_CHANNEL_H_

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
  Channel(const string& id);
  ~Channel();
  string GetId() const {return id_;}
  int GetUserCount() const {return users_.size();}
  void AddUser(User* user);
  void RemoveUser(User* user);
  void Broadcast(const string& content);

 private:
  string id_;
  map<string, User*> users_;
};
}  // namespace xcomet
#endif  // SRC_CHANNEL_H_
