#ifndef SRC_WEBSOCKET_WEBSOCKET_SESSION_H_
#define SRC_WEBSOCKET_WEBSOCKET_SESSION_H_

#include <evhttp.h>
#include "deps/base/basictypes.h"
#include "src/include_std.h"
#include "src/session.h"
#include "src/websocket/websocket.h"

namespace xcomet {

class WebSocketSession : public Session {
 public:
  WebSocketSession(struct evhttp_request* req);
  virtual ~WebSocketSession();
  virtual void Send(const Message& msg);
  virtual void Send(const string& packet_str);
  virtual void SendHeartbeat();
  virtual void Close();

 private:
  static void OnReceive(void* ctx);
  static void OnDisconnect(void* ctx);

  void Start();
  void Close(int16 reason);

  struct evhttp_request* req_;
  bool closed_;
  WebSocket ws_;
  string recv_buffer_;
};

}  // namespace xcomet

#endif  // SRC_WEBSOCKET_WEBSOCKET_SESSION_H_
