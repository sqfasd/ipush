#include "deps/base/string_util.h"
#include "src/router_proxy.h"
#include "src/http_query.h"

namespace xcomet {

RouterProxy::RouterProxy()
    : counter_(0) {
}

RouterProxy::~RouterProxy() {
}

void RouterProxy::OnSessionDisconnected() {
  LOG(ERROR) << "RouterProxy disconnected";
  session_.reset();
}

void RouterProxy::ResetSession(struct evhttp_request* req) {
  session_.reset(new Session(req));
  session_->SetDisconnectCallback(
      base::NewOneTimeCallback(this, &RouterProxy::OnSessionDisconnected));
}

void RouterProxy::RegisterUser(const string& uid, int seq) {
  LOG(INFO) << "RegisterUser: " << uid;
  string msg = StringPrintf("{\"type\":\"login\", \"uid\":\"%s\"}",
      uid.c_str());
  if (session_.get()) {
    session_->SendPacket(msg);
  } else {
    LOG(ERROR) << "session is not available";
  }
}

void RouterProxy::UnregisterUser(const string& uid) {
  string msg = StringPrintf("{\"type\":\"logout\", \"uid\":\"%s\"}",
      uid.c_str());
  if (session_.get()) {
    session_->SendPacket(msg);
  } else {
    LOG(ERROR) << "session is not available";
  }
}

void RouterProxy::SendHeartbeat() {
  if (session_.get()) {
    session_->SendHeartbeat();
  }
}

}  // namespace xcomet
