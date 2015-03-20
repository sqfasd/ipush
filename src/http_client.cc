#include "http_client.h"
#include <string.h>
#include "deps/jsoncpp/include/json/json.h"
#include "deps/base/flags.h"
#include "deps/base/logging.h"
#include "router_server.h"
#include "deps/base/time.h"
#include "deps/limonp/StringUtil.hpp"

DEFINE_int32(retry_interval, 2, "seconds");

namespace xcomet {

const struct timeval RETRY_TV = {FLAGS_retry_interval, 0};

using std::string;

HttpClient::HttpClient(struct event_base* evbase,
    const HttpClientOption& option, void *cb_arg)
    : evbase_(evbase),
      evconn_(NULL),
      evreq_(NULL),
      option_(option),
      cb_arg_(cb_arg) {
}

HttpClient::~HttpClient() {
  Free();
}

void HttpClient::Init() {
  VLOG(5) << option_;
  evconn_ = evhttp_connection_base_new(evbase_,
                                       NULL,
                                       option_.host.c_str(),
                                       option_.port);
  CHECK(evconn_);
  evhttp_connection_set_closecb(evconn_, OnClose, this);
}

void HttpClient::Free() {
  if(evconn_ != NULL) {
    VLOG(5) << "evhttp_connection_free";
    evhttp_connection_free(evconn_);
  }
}

void HttpClient::Send(const string& data) {
  struct evbuffer* buf = evhttp_request_get_output_buffer(evreq_);
  evbuffer_add_printf(buf, "%s\n", data.c_str());
  evhttp_send_reply_chunk(evreq_, buf);
  VLOG(6) << "Send to session:" << data;
}

void HttpClient::StartRequest() {
  VLOG(5) << "HttpClient::StartRequest()";
  evreq_ = evhttp_request_new(&HttpClient::OnRequestDone, this);
  evhttp_request_set_chunked_cb(evreq_, &HttpClient::OnChunk);
  CHECK(evreq_);
  struct evkeyvalq* output_headers = evhttp_request_get_output_headers(evreq_);
  evhttp_add_header(output_headers, "Host", option_.host.c_str());

  enum evhttp_cmd_type cmd = option_.method;
  if (cmd == EVHTTP_REQ_POST && !option_.data.empty()) {
    evhttp_add_header(evreq_->output_headers, "Content-Type", "text/json; charset=utf-8");
    Send(option_.data);
  }

  const char* uri = option_.path.empty() ? "/" : option_.path.c_str();
  int ret = evhttp_make_request(evconn_, evreq_, cmd, uri);
  if(ret != 0) {
    LOG(ERROR) << "evhttp_make_request failed!!!";
  }
}

void HttpClient::DelayRetry(EventCallback cb) {
  VLOG(5) << "HttpClient::DelayRetry";
  struct event * ev = event_new(evbase_, -1, EV_READ|EV_TIMEOUT, cb, this);
  event_add(ev, &RETRY_TV);
  LOG(INFO) << "make resubevent : timeout " << FLAGS_retry_interval;
}

void HttpClient::OnRetry(int sock, short which, void * ctx) {
  VLOG(5) << "HttpClient::OnRetry";
  HttpClient* self = (HttpClient*)ctx;
  CHECK(self);
  self->StartRequest();
}

bool HttpClient::IsResponseOK(evhttp_request* req) {
  int errcode;
  int code;
  if (req == NULL) {
    errcode = EVUTIL_SOCKET_ERROR();
    LOG(ERROR) << "socket error :" << evutil_socket_error_to_string(errcode);
    return false;
  }
  code = evhttp_request_get_response_code(req);
  if (code == 0) {
    errcode = EVUTIL_SOCKET_ERROR();
    LOG(ERROR) << "socket error :" << evutil_socket_error_to_string(errcode);
    return false;
  }
  
  if (code != 200 ) {
    LOG(ERROR) << "http response not ok.";
    return false;
  }

  return true;
}

void HttpClient::OnRequestDone(struct evhttp_request* req, void* ctx) {
  VLOG(5) << "HttpClient::OnRequestDone";
  HttpClient* self = (HttpClient*)ctx;
  CHECK(self);

  string buffer;
  if(IsResponseOK(req)) {
    VLOG(5) << "IsResponseOK";
    struct evbuffer* evbuf = evhttp_request_get_input_buffer(req);   
    int len = evbuffer_get_length(evbuf);
    buffer.resize(len);
    int nread = evbuffer_remove(evbuf, (char*)buffer.c_str(), buffer.size());
    CHECK(nread == len);
    VLOG(5) << "buffer: " << buffer;
  }
  
  if (self->request_done_cb_) {
    self->request_done_cb_(self, buffer, self->cb_arg_);
  }
}

void HttpClient::OnChunk(struct evhttp_request* req, void* ctx) {
  VLOG(5) << "HttpClient::OnChunk";
  HttpClient* self = (HttpClient*)ctx;
  struct evbuffer* evbuf = evhttp_request_get_input_buffer(req);   
  int len = evbuffer_get_length(evbuf);
  string buffer;
  buffer.resize(len);
  int nread = evbuffer_remove(evbuf, (char*)buffer.c_str(), buffer.size());
  CHECK(len == nread);
  if (self->chunk_cb_) {
    self->chunk_cb_(self, buffer, self->cb_arg_);
  }
}

void HttpClient::OnClose(struct evhttp_connection* conn, void *ctx) {
  VLOG(5) << "HttpClient::OnClose";
  HttpClient *self = (HttpClient*)ctx;
  if (self->close_cb_) {
    self->close_cb_(self, self->cb_arg_);
  }
}

} // namespace xcomet
