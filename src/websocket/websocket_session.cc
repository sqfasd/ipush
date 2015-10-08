#include "src/websocket/websocket_session.h"

namespace xcomet {

const int16 WS_CR_NONE = 0;
const int16 WS_CR_NORMAL = 1000;
const int16 WS_CR_PROTO_ERR = 1002;
const int16 WS_CR_DATA_TOO_BIG = 1009;

const int WS_RECV_BUFFER_SIZE = 4096;

static void GetHeader(struct evkeyvalq* headers, const char* k, string& v) {
  const char* ret = evhttp_find_header(headers, k);
  if (ret != NULL) {
    v.assign(ret);
  }
}

WebSocketSession::WebSocketSession(struct evhttp_request* req)
    : req_(req),
      closed_(false) {
  VLOG(3) << "WebSocketSession construct";
  struct evkeyvalq* headers = evhttp_request_get_input_headers(req);
  GetHeader(headers, "Host", ws_.host);
  GetHeader(headers, "Origin", ws_.origin);
  GetHeader(headers, "Sec-WebSocket-Key", ws_.key);
  GetHeader(headers, "Sec-WebSocket-Protocol", ws_.protocol);
  VLOG(3) << "\nHost: " << ws_.host
          << "\nOrigin: " << ws_.origin
          << "\nSec-WebSocket-Key: " << ws_.key
          << "\nSec-WebSocket-Protocol: " << ws_.protocol;
  VLOG(3) << "remote host: " << req->remote_host
          << ", remote port: " << req->remote_port;
  Start();
}

void WebSocketSession::Start() {
  string answer = ws_.answerHandshake();
  VLOG(3) << "ws answer:\n" << answer;
  evhttp_add_header(req_->output_headers, "Upgrade", "websocket");
  evhttp_add_header(req_->output_headers, "Connection", "Upgrade");
  evhttp_add_header(req_->output_headers,
                    "Sec-WebSocket-Accept",
                    ws_.getAcceptKey().c_str());
  if (!ws_.protocol.empty()) {
    evhttp_add_header(req_->output_headers,
                      "Sec-WebSocket-Protocol",
                      ws_.getProtocol().c_str());
  }
  evhttp_send_reply_start_ws(req_, 101, "Switching Protocols", OnReceive, this);
}

WebSocketSession::~WebSocketSession() {
  if (!closed_) {
    Close();
  }
}

void WebSocketSession::Send(const Message& msg) {
  StringPtr data = Message::Serialize(msg);
  if (data->empty()) {
    LOG(ERROR) << "invalid msg: " << msg;
  } else {
    Send(*data);
  }
}

void WebSocketSession::Send(const string& packet_str) {
  unsigned char frame[WS_RECV_BUFFER_SIZE] = {0};
  int len = ws_.makeFrame(TEXT_FRAME,
                          (unsigned char*)packet_str.c_str(),
                          packet_str.length(),
                          frame,
                          sizeof(frame));
  VLOG(6) << "WebSocketSession send buffer len: " << len;
  VLOG(6) << "WebSocketSession send buffer: " << packet_str;
  if (len <= 0) {
    LOG(WARNING) << "make websocket frame failed";
    return;
  }
  struct evbuffer* evbuf = evhttp_request_get_output_buffer(req_);
  evbuffer_add(evbuf, frame, len);
  evhttp_send_ws(req_, evbuf);
}

void WebSocketSession::SendHeartbeat() {
}

void WebSocketSession::Close() {
  VLOG(3) << "WebSocketSession close";
  Close(WS_CR_NORMAL);
}

void WebSocketSession::Close(int16 reason) {
  VLOG(3) << "WebSocketSession close reason: " << reason;
  if (closed_) {
    return;
  }
  closed_ = true;
  CHECK(req_);
  if (req_->evcon) {
    evhttp_connection_set_closecb(req_->evcon, NULL, NULL);
  }
  evhttp_end_ws(req_, reason);
}

void WebSocketSession::OnReceive(void* ctx) {
  VLOG(6) << "WebSocketSession OnReceive";
  WebSocketSession* self = static_cast<WebSocketSession*>(ctx);
  struct bufferevent* bev = evhttp_connection_get_bufferevent(self->req_->evcon);
  struct evbuffer* input = bufferevent_get_input(bev);
  int len = evbuffer_get_length(input);
  VLOG(6) << "receive len: " << len;
  string raw;
  raw.resize(len);
  evbuffer_remove(input, (char*)raw.c_str(), len);

  if (!self->recv_buffer_.empty()) {
    self->recv_buffer_ += raw;
  } else {
    self->recv_buffer_.swap(raw);
  }

  VLOG(6) << "recv_buffer_ size:" << self->recv_buffer_.size();

  if (self->recv_buffer_.length() > WS_RECV_BUFFER_SIZE) {
    LOG(WARNING) << "exceed the max recv buffer size: "
                 << self->recv_buffer_.length();
    OnDisconnect(self);
  }

  unsigned char msg_buf[WS_RECV_BUFFER_SIZE] = {0};
  WebSocketFrameType type = self->ws_.getFrame(
      (unsigned char*)self->recv_buffer_.c_str(),
      self->recv_buffer_.length(),
      msg_buf,
      sizeof(msg_buf),
      &len);
  VLOG(6) << "getFrame type: " << type;
  VLOG(6) << "getFrame len: " << len;
  VLOG(6) << "getFrame: [" << msg_buf << "]";

  // TODO(qingfeng) support other frame type
  // TODO(qingfeng) support multi frame data
  if (type == TEXT_FRAME) {
    if (self->message_callback_) {
      StringPtr msg(new string());
      msg->assign((char*)msg_buf, len);
      self->message_callback_(msg);
      self->recv_buffer_.clear();
    }
  } else if (type == INCOMPLETE_TEXT_FRAME ||
             type == INCOMPLETE_FRAME) {
    VLOG(3) << "incomplete frame received, wait for next chunk";
  } else if (type == INCOMPLETE_BINARY_FRAME ||
             type == BINARY_FRAME ||
             type == CLOSING_FRAME ||
             type == ERROR_FRAME) {
    OnDisconnect(self);
  } else if (type == PING_FRAME) {
    VLOG(6) << "ping frame";
  } else if (type == PONG_FRAME) {
    VLOG(6) << "PONG frame";
  } else {
    LOG(WARNING) << "unexpected frame type: " << type;
    OnDisconnect(self);
  }
}

void WebSocketSession::OnDisconnect(void* ctx) {
  VLOG(3) << "WebSocketSession OnDisconnect";
  WebSocketSession* self = static_cast<WebSocketSession*>(ctx);
  self->Close(WS_CR_NONE);
  if (self->disconnect_callback_) {
    self->disconnect_callback_();
  }
}

}  // namespace xcomet
