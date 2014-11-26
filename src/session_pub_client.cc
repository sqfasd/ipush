#include "session_pub_client.h"

#include "deps/jsoncpp/include/json/json.h"
#include "deps/base/flags.h"
#include "deps/base/logging.h"
#include "router_server.h"
#include "deps/base/time.h"
#include "deps/limonp/StringUtil.hpp"

DEFINE_int32(pub_read_buffer_size,  4096, "");

namespace xcomet {

using Limonp::string_format;

SessionPubClient::SessionPubClient(
            class RouterServer* parent,
            struct event_base* evbase, 
            size_t client_id,
            const string& pub_host,
            int pub_port
            )
    : 
        parent_(parent),
        evbase_(evbase), 
        client_id_(client_id),
        pub_host_(pub_host),
        pub_port_(pub_port),
        evhttpcon_(NULL) {
  InitConn();
}

SessionPubClient::~SessionPubClient() {
  CloseConn();
}

void SessionPubClient::MakePubEvent(const char* pub_uri) {
  VLOG(5) << "MakePubEvent";
  struct evhttp_request* req = evhttp_request_new(PubDoneCB, this);
  evhttp_request_set_error_cb(req, ReqErrorCB);
  evhttp_request_set_on_complete_cb(req, PubCompleteCB, this);
  CHECK(pub_uri);
  int r = evhttp_make_request(evhttpcon_, req, EVHTTP_REQ_GET, pub_uri);
  CHECK(r == 0);
}

void SessionPubClient::InitConn() {
  //struct bufferevent* bev = bufferevent_socket_new(evbase_, -1, 0);
  VLOG(5) << "init connections , pub_host: " 
          << pub_host_ 
          << " pub_port: " 
          << pub_port_;
  evhttpcon_ = evhttp_connection_base_bufferevent_new(
              evbase_, 
              NULL, 
              NULL, 
              pub_host_.c_str(),
              pub_port_);
  CHECK(evhttpcon_);
  evhttp_connection_set_closecb(evhttpcon_, ConnCloseCB, this);
  //evhttp_connection_set_retries(evhttpcon_, -1); // retry infinitely 
}

void SessionPubClient::CloseConn() {
  evhttp_connection_free(evhttpcon_);
}


void SessionPubClient::ConnCloseCB(struct evhttp_connection* conn, void *ctx) {
  VLOG(5) << "ConnCloseCB";
}

void SessionPubClient::PubDoneCB(struct evhttp_request* req, void * ctx) {
  VLOG(5) << "PubDoneCB";
  SessionPubClient* that = static_cast<SessionPubClient*>(ctx);
  char buffer[FLAGS_pub_read_buffer_size];
  int nread;

  if (req == NULL || evhttp_request_get_response_code(req) != 200) {
    int errcode = EVUTIL_SOCKET_ERROR();
    LOG(ERROR) << "socket error :" << evutil_socket_error_to_string(errcode);
    return;
  }

  while ((nread = evbuffer_remove(evhttp_request_get_input_buffer(req), buffer, sizeof(buffer))) > 0) {
    VLOG(5) << string(buffer, nread);
  }
}

void SessionPubClient::ReqErrorCB(enum evhttp_request_error err, void * ctx) {
  switch(err) {
    case EVREQ_HTTP_TIMEOUT:
      LOG(ERROR) << "EVREQ_HTTP_TIMEOUT";
      break;
    default:
      LOG(ERROR) << "ReqError hanppend"; 
  }
  //switch err {
  //case EVREQ_HTTP_TIMEOUT:
  //EVREQ_HTTP_EOF,
  //EVREQ_HTTP_INVALID_HEADER,
  //EVREQ_HTTP_BUFFER_ERROR,
  //EVREQ_HTTP_REQUEST_CANCEL,
  //EVREQ_HTTP_DATA_TOO_LONG 
}

void SessionPubClient::PubCompleteCB(struct evhttp_request* req, void * ctx) {
  VLOG(5) << "PubCompleteCB";
}

} // namespace xcomet
