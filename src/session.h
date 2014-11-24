#ifndef SRC_SESSION_H_
#define SRC_SESSION_H_

#include <evhttp.h>
#include "src/include_std.h"
#include "deps/base/callback.h"
namespace xcomet {
class Session {
 public:
  Session(struct evhttp_request* req);
  ~Session();
  void Send(const string& content);
  void Send2(const string& content);
  void SendHeartbeat();
  void Close();
  void Reset(struct evhttp_request* req);
  void SetDisconnectCallback(base::Closure* cb) {
    disconnect_callback_ = cb;
  }

 private:
  static void OnDisconnect(struct evhttp_connection* evconn, void* arg);
  void SendHeader();
  void SendChunk(const string& type, const string& content);

  struct evhttp_request* req_;
  bool closed_;
  base::Closure* disconnect_callback_;
};
}  // namespace xcomet
#endif  // SRC_SESSION_H_
