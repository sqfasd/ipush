#include "src/session_proxy.h"
#include "src/worker.h"

namespace xcomet {

const int RETRY_INTERVAL_SEC = 2;

SessionProxy::SessionProxy(struct event_base* evbase, int id) : id_(id) {
  ClientOption option;
  option.username = "default";
  option.password = "default";
  connection_.reset(new SocketClient(option));
  connection_->SetDataCallback(boost::bind(&SessionProxy::OnData, this, _1));
  worker_.reset(new Worker(evbase));
}

SessionProxy::~SessionProxy() {
}

void SessionProxy::SendData(const string& data) {
  connection_->Send(data);
}

void SessionProxy::SendMessage(MessagePtr msg) {
  SendData(Message::Serialize(msg));
}

void SessionProxy::StartConnect() {
  LOG(INFO) << "SessionProxy::StartConnect id: " << GetId()
            << ", connection option: " << connection_->GetOption();
  CHECK(connection_->Connect() == 0);
}

void SessionProxy::Retry() {
  worker_->Do(boost::bind(&SessionProxy::DoRetry, this),
              boost::bind(&SessionProxy::Retry, this));
}

void SessionProxy::DoRetry() {
  CHECK(!connection_->IsConnected());
  ::sleep(RETRY_INTERVAL_SEC);
  if (connection_->Connect() != 0) {
    LOG(ERROR) << "proxy " << GetId() << " connect failed, will retry again";
    Retry();
  } else {
    LOG(INFO) << "reconnect success";
  }
}

void SessionProxy::Close() {
  connection_->Close();
}

void SessionProxy::OnData(string& data) {
  if (msg_cb_) {
    StringPtr sptr(new string());
    sptr->swap(data);
    worker_->Do<MessagePtr>(boost::bind(Message::Unserialize, sptr), msg_cb_);
  }
}

}  // namespace xcomet
