#ifndef SRC_ROUTER_PROXY_H_
#define SRC_ROUTER_PROXY_H_

#include <evhttp.h>
#include "deps/base/logging.h"
#include "deps/base/scoped_ptr.h"
#include "deps/base/shared_ptr.h"
#include "deps/base/callback.h"
#include "src/include_std.h"
#include "src/session.h"

namespace xcomet {

class RouterProxy {
 public:
  typedef map<string, queue<string> > MsgMap;
  RouterProxy();
  ~RouterProxy();
  void SendHeartbeat();
  void Redirect(const string& uid, const string& content);
  void ResetSession(struct evhttp_request* req);
  void Redirect(struct evhttp_request* pub_req);
  void RegisterUser(const string& uid);
  void UnregisterUser(const string& uid);
  void ChannelBroadcast(const string& cid, const string& content);
  queue<string>* GetOfflieMessages(const string& uid);
  bool HasOfflineMessage(const string& uid);
  void RemoveOfflineMessages(const string& uid);
 private:
  void SendAll();
  void OnSessionDisconnected();
  scoped_ptr<Session> session_;
  MsgMap user_msgs_;
  set<string> users_;
  vector<string> msg_buffer_;
  const int buffer_size_;
  int seq_;
  int offset_;
  int tail_;
};
}  // namespace xcomet
#endif  // SRC_ROUTER_PROXY_H_
