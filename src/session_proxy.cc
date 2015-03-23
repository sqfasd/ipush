#include "src/session_proxy.h"
#include "src/worker.h"

namespace xcomet {

const int RETRY_INTERVAL_SEC = 2;

SessionProxy::SessionProxy(struct event_base* evbase, int id)
    : id_(id), stoped_(false) {
  ClientOption option;
  option.username = "default";
  option.password = "default";
  connection_.reset(new SocketClient(option));
  connection_->SetDataCallback(boost::bind(&SessionProxy::OnData, this, _1));
  connection_->SetErrorCallback(boost::bind(&SessionProxy::OnError, this, _1));
  connection_->SetConnectCallback(boost::bind(&SessionProxy::OnConnect, this));
  connection_->SetDisconnectCallback(
      boost::bind(&SessionProxy::OnDisconnect, this));
  worker_.reset(new Worker(evbase));
}

SessionProxy::~SessionProxy() {
}

void SessionProxy::SendData(string& data) {
  connection_->Send(data);
}

void SessionProxy::StartConnect() {
  LOG(INFO) << "SessionProxy::StartConnect id: " << GetId()
            << ", connection option: " << connection_->GetOption();
  if (connection_->Connect() != 0) {
    LOG(ERROR) << "failed to connect, will retry";
    Retry();
  }
}

void SessionProxy::Retry() {
  if (connection_->IsConnected() || stoped_) {
    return;
  }
  worker_->Do(boost::bind(&SessionProxy::DoRetry, this),
              boost::bind(&SessionProxy::Retry, this));
}

void SessionProxy::DoRetry() {
  ::sleep(RETRY_INTERVAL_SEC);
  if (stoped_) {
    return;
  }
  if (connection_->Connect() != 0) {
    LOG(ERROR) << "proxy " << GetId() << " connect failed, will retry again";
  } else {
    LOG(INFO) << "reconnect success";
  }
}

void SessionProxy::StopRetry() {
  stoped_ = true;
}

void SessionProxy::Close() {
  connection_->Close();
}

void SessionProxy::OnData(string& data) {
  if (msg_cb_) {
    StringPtr sptr(new string());
    sptr->swap(data);
    worker_->Do<MessagePtr>(boost::bind(Message::Unserialize, sptr),
                            boost::bind(msg_cb_, sptr, _1));
  }
}

void SessionProxy::OnError(const string& error) {
  LOG(ERROR) << "SessionProxy::OnError: " << error;
}

void SessionProxy::OnConnect() {
  if (connect_cb_) {
    worker_->RunInMainLoop(connect_cb_);
  }
}

void SessionProxy::OnDisconnect() {
  if (disconnect_cb_) {
    worker_->RunInMainLoop(disconnect_cb_);
  }
}

void SessionProxy::WaitForClose() {
  connection_->WaitForClose();
}

}  // namespace xcomet
