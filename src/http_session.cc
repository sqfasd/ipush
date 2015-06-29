#include "src/http_session.h"

#include "deps/jsoncpp/include/json/json.h"
#include "base/string_util.h"
#include "src/session_server.h"

namespace xcomet {

HttpSession::HttpSession(struct evhttp_request* req)
    : req_(req),
      closed_(false),
      next_msg_max_len_(0) {
  SendHeader();
}

HttpSession::~HttpSession() {
  if (!closed_) {
    Close();
  }
}

void HttpSession::Send(const Message& msg) {
  StringPtr data = Message::Serialize(msg);
  if (data->empty()) {
    LOG(ERROR) << "invalid msg: " << msg;
  } else {
    SendChunk(data->c_str(), false);
  }
}

void HttpSession::Send(const std::string& packet_str) {
  SendChunk(packet_str.c_str(), false);
}

void HttpSession::SendHeartbeat() {
  SendChunk("{\"type\":\"noop\"}");
}

void HttpSession::SendChunk(const char* data, bool new_line) {
  struct evbuffer* buf = evhttp_request_get_output_buffer(req_);
  if (new_line) {
    evbuffer_add_printf(buf, "%s\n", data);
  } else {
    evbuffer_add_printf(buf, "%s", data);
  }
  evhttp_send_reply_chunk_bi(req_, buf);
}

void HttpSession::Close() {
  closed_ = true;
  CHECK(req_);
  if (req_->evcon) {
    evhttp_connection_set_closecb(req_->evcon, NULL, NULL);
  }
  evhttp_send_reply_end(req_);
}

void HttpSession::OnDisconnect(struct evhttp_connection* evconn, void* arg) {
  HttpSession* self = (HttpSession*)arg;
  self->Close();
  if (self->disconnect_callback_) {
    self->disconnect_callback_();
  }
}

struct bufferevent* HttpSession::GetBufferEvent() {
  CHECK(req_ != NULL);
  CHECK(req_->evcon);
  CHECK(req_->output_headers);
  struct bufferevent* bev = evhttp_connection_get_bufferevent(req_->evcon);
  CHECK(bev != NULL);
  return bev;
}

void HttpSession::OnReceive(void* arg) {
  HttpSession* self = (HttpSession*)arg;
  self->OnReceive();
}

void HttpSession::OnReceive() {
  struct bufferevent* bev = GetBufferEvent();
  struct evbuffer* input = bufferevent_get_input(bev);
  while (true) {
    if (next_msg_max_len_ == 0) {
      char* size = evbuffer_readln(input, NULL, EVBUFFER_EOL_ANY);
      while (size != NULL) {
        VLOG(6) << "message len: " << size;
        next_msg_max_len_ = HexStringToInt(size);
        if (next_msg_max_len_ > 0) {
          break;
        }
        free(size);
        size = evbuffer_readln(input, NULL, EVBUFFER_EOL_ANY);
      }
      if (size != NULL) {
        free(size);
      } else {
        break;
      }
    }
    int left_len = evbuffer_get_length(input);
    if (left_len <= 0) {
      break;
    }
    VLOG(5) << "total buffer length: " << left_len;
    left_len -= next_msg_max_len_;
    if (left_len >= 0) {
      shared_ptr<string> message(new string());
      message->resize(next_msg_max_len_);
      evbuffer_remove(input, (char*)message->c_str(), next_msg_max_len_);
      if (message_callback_) {
        message_callback_(message);
      }
      next_msg_max_len_ = 0;
    } else {
      break;
    }
  }
}

void HttpSession::SendHeader() {
  evhttp_connection_set_closecb(req_->evcon, OnDisconnect, this);

  evhttp_add_header(req_->output_headers, "Connection", "keep-alive");
  evhttp_add_header(req_->output_headers, "Content-Type", "text/json; charset=utf-8");
  evhttp_send_reply_start_bi(req_, HTTP_OK, "OK", OnReceive, this);
}

void HttpSession::Reset(struct evhttp_request* req) {
  closed_ = false;
  req_ = req;
  SendHeader();
}

}  // namespace xcomet
