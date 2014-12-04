#include "session_sub_client.h"

#include "deps/jsoncpp/include/json/json.h"
#include "deps/base/flags.h"
#include "deps/base/logging.h"
#include "router_server.h"
#include "deps/base/time.h"
#include "deps/limonp/StringUtil.hpp"

DEFINE_int32(sub_read_buffer_size,  4096, "");

namespace xcomet {

using Limonp::string_format;

SessionSubClient::SessionSubClient(
            class RouterServer* parent,
            struct event_base* evbase, 
            size_t client_id,
            const string& sub_host,
            int sub_port, 
            const string& sub_uri
            )
    : 
        parent_(parent),
        evbase_(evbase), 
        client_id_(client_id),
        sub_host_(sub_host),
        sub_port_(sub_port),
        sub_uri_(sub_uri),
        evhttpcon_(NULL) {
  InitConn();
}

SessionSubClient::~SessionSubClient() {
  CloseConn();
}

void SessionSubClient::MakeSubEvent() {
  VLOG(5) << "MakeSubEvent";
  struct evhttp_request* req = evhttp_request_new(SubDoneCB, this);
  evhttp_request_set_chunked_cb(req, SubChunkCB);
  //evhttp_request_set_error_cb(req, ReqErrorCB);
  //evhttp_request_set_on_complete_cb(req, SubCompleteCB, this);
  CHECK(sub_uri_.size());
  int r = evhttp_make_request(evhttpcon_, req, EVHTTP_REQ_GET, sub_uri_.c_str());
  CHECK(r == 0);
}

void SessionSubClient::InitConn() {
  VLOG(5) << "init connections , sub_host: " 
          << sub_host_ 
          << " sub_port: " 
          << sub_port_;
  evhttpcon_ = evhttp_connection_base_new(
              evbase_, 
              NULL, 
              sub_host_.c_str(),
              sub_port_);
  CHECK(evhttpcon_);
  evhttp_connection_set_closecb(evhttpcon_, ConnCloseCB, this);
  //evhttp_connection_set_retries(evhttpcon_, -1); // retry infinitely 
}

void SessionSubClient::CloseConn() {
  evhttp_connection_free(evhttpcon_);
}


void SessionSubClient::ConnCloseCB(struct evhttp_connection* conn, void *ctx) {
  VLOG(5) << "ConnCloseCB";
}

void SessionSubClient::SubDoneCB(struct evhttp_request *req, void *ctx) {
  VLOG(5) << "SessionSubClient::SubDoneCB";
  SessionSubClient* self = static_cast<SessionSubClient*>(ctx);
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

  self->parent_->MakeCliErrEvent(new CliErrInfo(self->client_id_, "socket error", self->parent_));
}

void SessionSubClient::SubChunkCB(struct evhttp_request* req, void * ctx) {
  SessionSubClient* self = static_cast<SessionSubClient*>(ctx);
  VLOG(5) << "enter SubChunkCB";
  char buffer[FLAGS_sub_read_buffer_size];
  int nread;

  while((nread = evbuffer_remove(evhttp_request_get_input_buffer(req), buffer, sizeof(buffer))) > 0) {
    VLOG(5) << "read buffer size: " << nread;
    VLOG(5) << "read buffer date: " << (string(buffer, nread - 1));
    self->parent_->ChunkedMsgHandler(self->client_id_, buffer, nread); 
  }
  VLOG(5) << "finished SubChunkCB";
}

//void SessionSubClient::ReqErrorCB(enum evhttp_request_error err, void * ctx) {
//  switch(err) {
//    case EVREQ_HTTP_TIMEOUT:
//      LOG(ERROR) << "EVREQ_HTTP_TIMEOUT";
//      break;
//    default:
//      LOG(ERROR) << "ReqError hanppend"; 
//  }
//  //switch err {
//  //case EVREQ_HTTP_TIMEOUT:
//  //EVREQ_HTTP_EOF,
//  //EVREQ_HTTP_INVALID_HEADER,
//  //EVREQ_HTTP_BUFFER_ERROR,
//  //EVREQ_HTTP_REQUEST_CANCEL,
//  //EVREQ_HTTP_DATA_TOO_LONG 
//}


void SessionSubClient::SubCompleteCB(struct evhttp_request* req, void * cxt) {
  VLOG(5) << "SubCompleteCB";
}

} // namespace xcomet
