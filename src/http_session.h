#ifndef SRC_HTTP_SESSION_H_
#define SRC_HTTP_SESSION_H_

#include <evhttp.h>
#include "src/session.h"

namespace xcomet {
class HttpSession : public Session {
 public:
  HttpSession(struct evhttp_request* req);
  virtual ~HttpSession();
  virtual void Send(const Message& msg);
  virtual void Send(const string& packet_str);
  virtual void SendHeartbeat();
  virtual void Close();
  void Reset(struct evhttp_request* req);

 private:
  struct bufferevent* GetBufferEvent();
  static void OnDisconnect(struct evhttp_connection* evconn, void* arg);
  static void OnReceive(void* arg);
  void OnReceive();
  void SendHeader();
  void SendChunk(const char* data, bool newline = true);

  struct evhttp_request* req_;
  bool closed_;
  int next_msg_max_len_;
};
}  // namespace xcomet
#endif  // SRC_HTTP_SESSION_H_
