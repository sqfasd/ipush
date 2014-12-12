#include "src/user.h"

#include "deps/base/logging.h"
#include "src/session_server.h"

namespace xcomet {
User::User(const string& uid,
           int type,
           struct evhttp_request* req,
           SessionServer& serv)
    : session_(req),
      prev_(NULL),
      next_(NULL),
      queue_index_(-1),
      uid_(uid),
      type_(type), 
      channel_id_("-1"),
      server_(serv) {
  VLOG(3) << "User construct";
  session_.SetDisconnectCallback(
      base::NewOneTimeCallback(this, &User::OnSessionDisconnected));
  session_.SetMessageCallback(boost::bind(&SessionServer::OnUserMessage,
                              &server_, uid_, _1));
}

User::~User() {
  VLOG(3) << "User destroy";
  // TODO if not closed, close it
}

void User::Send(const std::string& content) {
  session_.Send(content);
  if (type_ == COMET_TYPE_POLLING) {
    Close();
  }
}

void User::Send2(const string& content) {
  session_.Send2(content);
  if (type_ == COMET_TYPE_POLLING) {
    Close();
  }
}

void User::SendHeartbeat() {
  session_.SendHeartbeat();
  if (type_ == COMET_TYPE_POLLING) {
    Close();
  }
}

void User::Close() {
  LOG(INFO) << "User Close: " << uid_;
  session_.Close();
  server_.RemoveUser(this);
}

void User::OnSessionDisconnected() {
  LOG(INFO) << "User Disconnected: " << uid_;
  server_.RemoveUser(this);
}

}  // namespace xcomet
