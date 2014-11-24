#include "src/session.h"
#include "src/session_server.h"

namespace xcomet {

Session::Session(struct evhttp_request* req)
    : req_(req),
      closed_(false),
      disconnect_callback_(NULL) {
  SendHeader();
}

Session::~Session() {
  if (!closed_) {
    Close();
  }
}

void Session::Send(const std::string& content) {
  SendChunk("data", content);
}

void Session::Send2(const std::string& content) {
  struct evbuffer* buf = evbuffer_new();
  evbuffer_add_printf(buf, "%s\n", content.c_str());
  evhttp_send_reply_chunk(req_, buf);
  evbuffer_free(buf);
}

void Session::SendHeartbeat() {
  SendChunk("noop", "");
}

void Session::SendChunk(const string& type, const string& content) {
  struct evbuffer* buf = evbuffer_new();
  evbuffer_add_printf(buf, "{\"type\": \"%s\",\"content\":\"%s\"}\n",
                      type.c_str(), content.c_str());
  evhttp_send_reply_chunk(req_, buf);
  evbuffer_free(buf);
}

void Session::Close() {
  closed_ = true;
  CHECK(req_);
  if (req_->evcon) {
    evhttp_connection_set_closecb(req_->evcon, NULL, NULL);
  }
	evhttp_send_reply_end(req_);
}

void Session::OnDisconnect(struct evhttp_connection* evconn, void* arg) {
  Session* self = (Session*)arg;
  self->Close();
  if (self->disconnect_callback_) {
    self->disconnect_callback_->Run();
  }
}

void Session::SendHeader() {
  CHECK(req_ != NULL);
  CHECK(req_->evcon);
  CHECK(req_->output_headers);
	bufferevent_enable(evhttp_connection_get_bufferevent(req_->evcon), EV_READ);
	evhttp_connection_set_closecb(req_->evcon, OnDisconnect, this);
	evhttp_add_header(req_->output_headers, "Connection", "keep-alive");
	evhttp_add_header(req_->output_headers, "Content-Type", "text/html; charset=utf-8");
	evhttp_send_reply_start(req_, HTTP_OK, "OK");
}

void Session::Reset(struct evhttp_request* req) {
  closed_ = false;
  req_ = req;
  SendHeader();
}

}  // namespace xcomet
