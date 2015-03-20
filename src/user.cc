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
      server_(serv) {
  VLOG(3) << "User construct";
  session_.SetDisconnectCallback(
      base::NewOneTimeCallback(this, &User::OnSessionDisconnected));
  session_.SetMessageCallback(boost::bind(&SessionServer::OnUserMessage,
                              &server_, uid_, _1));
}

User::~User() {
  VLOG(3) << "User destroy";
}

void User::SendPacket(const std::string& pakcet_str) {
  session_.SendPacket(pakcet_str);
  if (type_ == COMET_TYPE_POLLING) {
    Close();
  }
}

void User::Send(const string& from_id,
                const string& type,
                const string& content) {
  session_.Send(from_id, type, content);
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
