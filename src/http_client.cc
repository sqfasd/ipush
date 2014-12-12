#include "http_client.h"
#include <string.h>
#include "deps/jsoncpp/include/json/json.h"
#include "deps/base/flags.h"
#include "deps/base/logging.h"
#include "router_server.h"
#include "deps/base/time.h"
#include "deps/limonp/StringUtil.hpp"

namespace xcomet {

using std::string;

HttpClient::HttpClient(struct event_base* evbase,
    const HttpClientOption& option, void *cb_arg)
    : evbase_(evbase),
      evconn_(NULL),
      evreq_(NULL),
      option_(option),
      cb_arg_(cb_arg) {
  VLOG(5) << option_;
  evconn_ = evhttp_connection_base_new(evbase_,
                                       NULL,
                                       option_.host.c_str(),
                                       option_.port);
  evreq_ = evhttp_request_new(&HttpClient::OnRequestDone, this);
  CHECK(evreq_);
  evhttp_request_set_chunked_cb(evreq_, &HttpClient::OnChunk);
  CHECK(evconn_);
  evhttp_connection_set_closecb(evconn_, OnClose, this);

}

HttpClient::~HttpClient() {
  if(evconn_ != NULL) {
    VLOG(5) << "evhttp_connection_free";
    evhttp_connection_free(evconn_);
  }
}

void HttpClient::Send(const string& data) {
  struct evbuffer* buf = evhttp_request_get_output_buffer(evreq_);
  evbuffer_add(buf, data.c_str(), data.size());
  VLOG(6) << "Send:" << data;
}

void HttpClient::SendChunk(const string& data) {
  struct evbuffer* buf = evhttp_request_get_output_buffer(evreq_);
  evbuffer_add_printf(buf, "%s\n", data.c_str());
  evhttp_send_reply_chunk(evreq_, buf);
  VLOG(6) << "SendChunk:" << data;
}

void HttpClient::StartRequest() {
  VLOG(5) << "HttpClient::StartRequest()";
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

void HttpClient::OnRequestDone(struct evhttp_request* evreq_, void* ctx) {
  VLOG(5) << "HttpClient::OnRequestDone";
  HttpClient* self = (HttpClient*)ctx;
  CHECK(self);

  int errcode;
  int code;

  do {
    if (evreq_ == NULL) {
      errcode = EVUTIL_SOCKET_ERROR();
      LOG(ERROR) << "socket error :" << evutil_socket_error_to_string(errcode);
      break;
    }
    code = evhttp_request_get_response_code(evreq_);
    if (code == 0) {
      errcode = EVUTIL_SOCKET_ERROR();
      LOG(ERROR) << "socket error :" << evutil_socket_error_to_string(errcode);
      break;
    }
    
    if (code != 200 ) {
      LOG(ERROR) << "http response not ok.";
      break;
    }
  } while(false);

  fprintf(stdout, "request sucess:\n");

  struct evbuffer* evbuf = evhttp_request_get_input_buffer(evreq_);   
  int len = evbuffer_get_length(evbuf);
  string buffer;
  buffer.resize(len);
  int nread = evbuffer_remove(evbuf, (char*)buffer.c_str(), buffer.size());
  CHECK(nread == len);
  
  if (self->request_done_cb_) {
    self->request_done_cb_(self, buffer, self->cb_arg_);
  }
}

void HttpClient::OnChunk(struct evhttp_request* evreq_, void* ctx) {
  VLOG(5) << "HttpClient::OnChunk";
  HttpClient* self = (HttpClient*)ctx;
  struct evbuffer* evbuf = evhttp_request_get_input_buffer(evreq_);   
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
