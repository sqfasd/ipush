#ifndef SRC_USER_H_
#define SRC_USER_H_

#include <evhttp.h>
#include "src/include_std.h"

namespace xcomet {

enum {
  STREAM = 1,
  POLLING,
};

class XCometServer;
class User {
 public:
  User(const string& uid, int type, struct evhttp_request* req, XCometServer& serv);
  ~User();
  void SetType(int type) {type_ = type;}
  string GetUid() {return uid_;}
  void Send(const string& content);
  void Close();
  void Start();
 private:
  static void OnDisconnect(struct evhttp_connection* evconn, void* arg);
  string uid_;
  int type_;
  struct evhttp_request* req_;
  XCometServer& server_;
};
}  // namespace xcomet
#endif  // SRC_USER_H_
