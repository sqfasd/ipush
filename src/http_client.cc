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
    const HttpClientOption& option)
    : evbase_(evbase),
      evconn_(NULL),
      option_(option) {
  evconn_ = evhttp_connection_base_new(evbase_,
                                       NULL,
                                       option_.host.c_str(),
                                       option_.port);
  CHECK(evconn_);
  evhttp_connection_set_closecb(evconn_, OnClose, this);
}

HttpClient::~HttpClient() {
  if(evconn_ != NULL) {
    VLOG(5) << "evhttp_connection_free";
    evhttp_connection_free(evconn_);
  }
}

void HttpClient::StartRequest() {
  struct evhttp_request* req = evhttp_request_new(&HttpClient::OnRequestDone, this);
  evhttp_request_set_chunked_cb(req, &HttpClient::OnChunk);

  struct evkeyvalq* output_headers = evhttp_request_get_output_headers(req);
  evhttp_add_header(output_headers, "Host", option_.host.c_str());

  enum evhttp_cmd_type cmd = option_.method;
  if (cmd == EVHTTP_REQ_POST && !option_.data.empty()) {
    evbuffer_add(req->output_buffer,
                 option_.data.c_str(),
                 option_.data.length());
    evhttp_add_header(output_headers,
                      "Content-Type",
                      "application/x-www-form-urlencoded");
    VLOG(6) << "post data: " << option_.data;
  }

  const char* uri = option_.path.empty() ? "/" : option_.path.c_str();
  int ret = evhttp_make_request(evconn_, req, cmd, uri);
  if(ret != 0) {
    LOG(ERROR) << "evhttp_make_request failed!!!";
  }
}

void HttpClient::OnRequestDone(struct evhttp_request* req, void* ctx) {
  VLOG(5) << "HttpClient::OnRequestDone";
  HttpClient* self = (HttpClient*)ctx;
  CHECK(self);

  int errcode;
  int code;

  do {
    if (req == NULL) {
      errcode = EVUTIL_SOCKET_ERROR();
      LOG(ERROR) << "socket error :" << evutil_socket_error_to_string(errcode);
      break;
    }
    code = evhttp_request_get_response_code(req);
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

  struct evbuffer* evbuf = evhttp_request_get_input_buffer(req);   
  int len = evbuffer_get_length(evbuf);
  string buffer;
  buffer.resize(len);
  int nread = evbuffer_remove(evbuf, (char*)buffer.c_str(), buffer.size());
  CHECK(nread == len);
  
  if (self->request_done_cb_) {
    self->request_done_cb_(self, buffer);
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
    self->chunk_cb_(self, buffer);
  }
}

void HttpClient::OnClose(struct evhttp_connection* conn, void *ctx) {
  VLOG(5) << "HttpClient::OnClose";
  HttpClient *self = (HttpClient*)ctx;
  if (self->close_cb_) {
    self->close_cb_(self);
  }
}

} // namespace xcomet
