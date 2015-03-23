#ifndef SRC_SESSION_PROXY_H_
#define SRC_SESSION_PROXY_H_

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include "deps/base/scoped_ptr.h"
#include "src/include_std.h"
#include "src/socketclient.h"
#include "src/message.h"

namespace xcomet {

class Worker;

typedef boost::function<void (StringPtr, MessagePtr)> MessageCallback;

class SessionProxy {
 public:
  SessionProxy(struct event_base* ev, int id);
  virtual ~SessionProxy();
  void SetMessageCallback(MessageCallback cb) {
    msg_cb_ = cb;
  }
  void SetConnectCallback(ConnectCallback cb) {
    connect_cb_ = cb;
  }
  void SetDisconnectCallback(DisconnectCallback cb) {
    disconnect_cb_ = cb;
  }
  void SetHost(const string& host) {connection_->SetHost(host);}
  void SetPort(int port) {connection_->SetPort(port);}
  void SetId(int id) {id_ = id;}
  int GetId() const {return id_;}
  void StartConnect();
  void Retry();
  void StopRetry();
  void SendData(string& data); // maybe swap
  void Close();
  void WaitForClose();
  bool IsConnected() const {
    return connection_.get() && connection_->IsConnected();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SessionProxy);

  void OnData(string& data);
  void OnError(const string& error);
  void OnDisconnect();
  void OnConnect();
  void RetryDone();
  void DoRetry();

  int id_;
  MessageCallback msg_cb_;
  ConnectCallback connect_cb_;
  DisconnectCallback disconnect_cb_;
  scoped_ptr<SocketClient> connection_;
  scoped_ptr<Worker> worker_;
  bool stoped_;
};
}  // namespace xcomet
#endif  // SRC_SESSION_PROXY_H_
