#ifndef SRC_SESSION_H_
#define SRC_SESSION_H_

#include <evhttp.h>
#include <boost/function.hpp>
#include "base/shared_ptr.h"
#include "src/include_std.h"
#include "deps/base/callback.h"

namespace xcomet {
typedef function<void (shared_ptr<string>)> MessageCallback;
class Session {
 public:
  Session(struct evhttp_request* req);
  virtual ~Session();
  void Send(Message msg);
  void Send(const string& packet_str);
  void Send(const string& from_id, const string& type, const string& content);
  void SendHeartbeat();
  void Close();
  void Reset(struct evhttp_request* req);
  void SetDisconnectCallback(base::Closure* cb) {
    disconnect_callback_ = cb;
  }
  void SetMessageCallback(MessageCallback cb) {
    message_callback_ = cb;
  }

 private:
  struct bufferevent* GetBufferEvent();
  static void OnDisconnect(struct evhttp_connection* evconn, void* arg);
  static void OnReceive(void* arg);
  void OnReceive();
  void SendHeader();
  void SendChunk(const char* data);

  struct evhttp_request* req_;
  bool closed_;
  base::Closure* disconnect_callback_;
  int next_msg_max_len_;
  MessageCallback message_callback_;
};
}  // namespace xcomet
#endif  // SRC_SESSION_H_
