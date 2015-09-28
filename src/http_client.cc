#include "deps/base/logging.h"
#include "src/http_client.h"
#include <string.h>

using std::string;

namespace xcomet {

HttpClient::HttpClient(struct event_base* evbase,
    const HttpRequestOption& option)
    : evbase_(evbase),
      evconn_(NULL),
      evreq_(NULL),
      option_(option) {
}

HttpClient::~HttpClient() {
  if (evconn_ != NULL) {
    evhttp_connection_free(evconn_);
  }
}

void HttpClient::StartRequest() {
  evconn_ = evhttp_connection_base_new(evbase_,
                                       NULL,
                                       option_.host,
                                       option_.port);

  evreq_ = evhttp_request_new(&HttpClient::OnRequestDone, this);
  // evhttp_request_set_chunked_cb(evreq_, &HttpClient::OnChunk);

  struct evkeyvalq* output_headers = evhttp_request_get_output_headers(evreq_);
  evhttp_add_header(output_headers, "Host", option_.host);

  enum evhttp_cmd_type cmd = EVHTTP_REQ_GET;
  if (::strcmp(option_.method, "post") == 0) {
    cmd = EVHTTP_REQ_POST;
  } else {
    // TODO other http methods
  }
  if (cmd == EVHTTP_REQ_POST && option_.data_len != 0) {
    evbuffer_add(evreq_->output_buffer,
                 option_.data,
                 option_.data_len);
    evhttp_add_header(output_headers,
                      "Content-Type",
                      "application/x-www-form-urlencoded");
  }

  const char* uri = "/";
  if (option_.path != NULL && ::strlen(option_.path) > 0) {
    uri = option_.path;
  }
  int ret = evhttp_make_request(evconn_, evreq_, cmd, uri);
  if (ret != 0) {
    if (request_done_cb_) {
      request_done_cb_("http request failed", StringPtr());
    }
  }
}

void HttpClient::OnRequestDone(struct evhttp_request* req, void* ctx) {
  HttpClient* self = (HttpClient*)ctx;
  int code = 0;
  if (req == NULL || (code = evhttp_request_get_response_code(req)) == 0) {
    int errcode = EVUTIL_SOCKET_ERROR();
    string err;
    err += "http request failed with internal error: ";
    err += evutil_socket_error_to_string(errcode);
    if (self->request_done_cb_) {
      self->request_done_cb_(err.c_str(), StringPtr());
    } else {
      LOG(ERROR) << err;
    }
    return;
  }
  struct evbuffer* evbuf = evhttp_request_get_input_buffer(req);
  int len = evbuffer_get_length(evbuf);
  char* body = (char*)evbuffer_pullup(evbuf, -1);
  if (code != 200) {
    if (self->request_done_cb_) {
      self->request_done_cb_(body, StringPtr());
    } else {
      LOG(ERROR) << "server response error code: " << code;
    }
    return;
  }

  std::shared_ptr<string> buffer(new string());
  buffer->reserve(len+1);
  buffer->append((char*)evbuffer_pullup(evbuf, -1), len);
  if (self->request_done_cb_) {
    self->request_done_cb_(NULL, buffer);
  }
}

void HttpClient::OnChunk(struct evhttp_request* req, void* ctx) {
  HttpClient* self = (HttpClient*)ctx;
  struct evbuffer* evbuf = evhttp_request_get_input_buffer(req);
  int len = evbuffer_get_length(evbuf);
  std::shared_ptr<string> buffer(new string());
  buffer->reserve(len+1);
  buffer->append((char*)evbuffer_pullup(evbuf, -1), len);
  if (self->chunk_cb_) {
    self->chunk_cb_(self, buffer);
  }
}

}  // namespace xcomet
