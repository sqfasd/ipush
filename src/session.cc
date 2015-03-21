#include "src/session.h"

#include "deps/jsoncpp/include/json/json.h"
#include "base/string_util.h"
#include "src/session_server.h"

namespace xcomet {

Session::Session(struct evhttp_request* req)
    : req_(req),
      closed_(false),
      disconnect_callback_(NULL),
      next_msg_max_len_(0) {
  SendHeader();
}

Session::~Session() {
  if (!closed_) {
    Close();
  }
}

void Session::SendPacket(const std::string& packet_str) {
  SendChunk(packet_str.c_str());
}

void Session::Send(const string& from_id,
                   const string& type,
                   const string& content) {
  Json::Value json;
  json["from"] = from_id;
  json["type"] = type;
  json["body"] = content;
  Json::FastWriter writer;
  SendChunk(writer.write(json).c_str());
}

void Session::SendHeartbeat() {
  SendChunk("{\"type\":\"noop\"}");
}

void Session::SendChunk(const char* data) {
  struct evbuffer* buf = evhttp_request_get_output_buffer(req_);
  evbuffer_add_printf(buf, "%s\r\n", data);
  evhttp_send_reply_chunk_bi(req_, buf);
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

struct bufferevent* Session::GetBufferEvent() {
  CHECK(req_ != NULL);
  CHECK(req_->evcon);
  CHECK(req_->output_headers);
  struct bufferevent* bev = evhttp_connection_get_bufferevent(req_->evcon);
  CHECK(bev != NULL);
  return bev;
}

void Session::OnReceive(void* arg) {
  Session* self = (Session*)arg;
  self->OnReceive();
}

void Session::OnReceive() {
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
      }
    }
    int left_len = evbuffer_get_length(input);
    if (left_len <= 0) {
      break;
    }
    LOG(INFO) << "total buffer length: " << left_len;
    left_len -= next_msg_max_len_;
    if (left_len >= 0) {
      base::shared_ptr<string> message(new string());
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

void Session::SendHeader() {
	evhttp_connection_set_closecb(req_->evcon, OnDisconnect, this);

	evhttp_add_header(req_->output_headers, "Connection", "keep-alive");
	evhttp_add_header(req_->output_headers, "Content-Type", "text/html; charset=utf-8");
	evhttp_send_reply_start_bi(req_, HTTP_OK, "OK", OnReceive, this);
}

void Session::Reset(struct evhttp_request* req) {
  closed_ = false;
  req_ = req;
  SendHeader();
}

}  // namespace xcomet
