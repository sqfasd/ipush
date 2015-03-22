#include "src/router_proxy.h"

#include <boost/bind.hpp>
#include "deps/base/string_util.h"
#include "src/http_query.h"
#include "src/session_server.h"

namespace xcomet {

RouterProxy::RouterProxy(SessionServer& serv)
    : counter_(0), server_(serv) {
}

RouterProxy::~RouterProxy() {
}

void RouterProxy::OnSessionDisconnected() {
  LOG(ERROR) << "RouterProxy disconnected";
  session_.reset();
}

void RouterProxy::Redirect(base::shared_ptr<string> message) {
  if (session_.get()) {
    session_->SendPacket(*message);
  } else {
    LOG(ERROR) << "Redirect failed: session is not available";
  }
}

void RouterProxy::ResetSession(struct evhttp_request* req) {
  session_.reset(new Session(req));
  session_->SetDisconnectCallback(
      base::NewOneTimeCallback(this, &RouterProxy::OnSessionDisconnected));
  session_->SetMessageCallback(
      boost::bind(&SessionServer::OnRouterMessage, &server_, _1));
}

void RouterProxy::LoginUser(const string& uid) {
  VLOG(3) << "RegisterUser: " << uid;
  string msg = StringPrintf("{\"type\":\"login\", \"from\":\"%s\"}",
      uid.c_str());
  if (session_.get()) {
    session_->SendPacket(msg);
  } else {
    LOG(ERROR) << "LoginUser failed: session is not available";
  }
}

void RouterProxy::LogoutUser(const string& uid) {
  string msg = StringPrintf("{\"type\":\"logout\", \"from\":\"%s\"}",
      uid.c_str());
  if (session_.get()) {
    session_->SendPacket(msg);
  } else {
    LOG(ERROR) << "LogoutUser failed: session is not available";
  }
}

void RouterProxy::SendHeartbeat() {
  if (session_.get()) {
    session_->SendHeartbeat();
  }
}

}  // namespace xcomet
