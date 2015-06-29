#ifndef SRC_SESSION_H_
#define SRC_SESSION_H_

#include "base/shared_ptr.h"
#include "src/include_std.h"
#include "src/message.h"

namespace xcomet {
typedef function<void (shared_ptr<string>)> MessageCallback;
typedef function<void ()> DisconnectCallback;
class Session {
 public:
  virtual ~Session() {}
  virtual void Send(const Message& msg) {}
  virtual void Send(const string& packet_str) {}
  virtual void SendHeartbeat() {}
  virtual void Close() {}

  void SetDisconnectCallback(DisconnectCallback cb) {
    disconnect_callback_ = cb;
  }

  void SetMessageCallback(MessageCallback cb) {
    message_callback_ = cb;
  }

 protected:
  DisconnectCallback disconnect_callback_;
  MessageCallback message_callback_;
};
}  // namespace xcomet
#endif  // SRC_SESSION_H_
