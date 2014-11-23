#include "src/user.h"
#include "src/session_server.h"

namespace xcomet {
User::User(const string& uid,
           int type,
           struct evhttp_request* req,
           SessionServer& serv)
    : prev_(NULL),
      next_(NULL),
      queue_index_(-1),
      uid_(uid),
      type_(type),
      req_(req),
      server_(serv) {
  VLOG(3) << "User construct";
	bufferevent_enable(evhttp_connection_get_bufferevent(req_->evcon), EV_READ);
	evhttp_connection_set_closecb(req_->evcon, OnDisconnect, this);
	evhttp_add_header(req_->output_headers, "Connection", "keep-alive");
	evhttp_add_header(req_->output_headers, "Content-Type", "text/html; charset=utf-8");
	evhttp_send_reply_start(req_, HTTP_OK, "OK");
}

User::~User() {
  VLOG(3) << "User destroy";
  // TODO if not closed, close it
}

void User::Send(const std::string& content) {
  SendChunk("data", content);
  if (type_ == COMET_TYPE_POLLING) {
    Close();
  }
}

void User::SendHeartbeat() {
  SendChunk("noop", "");
}

void User::SendChunk(const string& type, const string& content) {
  struct evbuffer* buf = evbuffer_new();
  evbuffer_add_printf(buf, "{\"type\": \"%s\",\"content\":\"%s\"}\n",
                      type.c_str(), content.c_str());
  evhttp_send_reply_chunk(req_, buf);
  evbuffer_free(buf);
}

void User::Close() {
  LOG(INFO) << "User connection disconnected: " << uid_;
  if (req_->evcon) {
    evhttp_connection_set_closecb(req_->evcon, NULL, NULL);
  }
	evhttp_send_reply_end(req_);
  server_.RemoveUser(this);
}

void User::OnDisconnect(struct evhttp_connection* evconn, void* arg) {
  User* self = (User*)arg;
  self->Close();
}

}  // namespace xcomet
