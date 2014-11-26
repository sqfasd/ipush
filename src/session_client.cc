#include "session_client.h"

#include "deps/jsoncpp/include/json/json.h"
#include "deps/base/flags.h"
#include "deps/base/logging.h"
#include "router_server.h"
#include "deps/base/time.h"
#include "deps/limonp/StringUtil.hpp"

DEFINE_int32(read_buffer_size,  4096, "");

namespace xcomet {

using Limonp::string_format;

SessionClient::SessionClient(
            class RouterServer* parent,
            struct event_base* evbase, 
            size_t client_id,
            const string& sub_host,
            int sub_port, 
            const string& sub_uri,
            const string& pub_host,
            int pub_port
            )
    : 
        parent_(parent),
        evbase_(evbase), 
        client_id_(client_id),
        sub_host_(sub_host),
        sub_port_(sub_port),
        sub_uri_(sub_uri),
        pub_host_(pub_host),
        pub_port_(pub_port),
        evhttpcon_(NULL) {
  InitConn();
}

SessionClient::~SessionClient() {
  CloseConn();
}

void SessionClient::MakeSubEvent() {
  VLOG(5) << "MakeSubEvent";
  struct evhttp_request* req = evhttp_request_new(SubDoneCB, this);
  evhttp_request_set_chunked_cb(req, SubChunkCB);
  evhttp_request_set_error_cb(req, ReqErrorCB);
  evhttp_request_set_on_complete_cb(req, SubCompleteCB, this);
  CHECK(sub_uri_.size());
  int r = evhttp_make_request(evhttpcon_, req, EVHTTP_REQ_GET, sub_uri_.c_str());
  CHECK(r == 0);
}

void SessionClient::MakePubEvent(const char* pub_uri) {
  VLOG(5) << "MakePubEvent";
  struct evhttp_request* req = evhttp_request_new(PubDoneCB, this);
  evhttp_request_set_chunked_cb(req, SubChunkCB);
  evhttp_request_set_error_cb(req, ReqErrorCB);
  evhttp_request_set_on_complete_cb(req, PubCompleteCB, this);
  CHECK(pub_uri);
  int r = evhttp_make_request(evhttpcon_, req, EVHTTP_REQ_GET, pub_uri);
  CHECK(r == 0);
}

void SessionClient::InitConn() {
  //struct bufferevent* bev = bufferevent_socket_new(evbase_, -1, 0);
  VLOG(5) << "init connections , sub_host: " 
          << sub_host_ 
          << " sub_port: " 
          << sub_port_;
  evhttpcon_ = evhttp_connection_base_bufferevent_new(
              evbase_, 
              NULL, 
              NULL, 
              sub_host_.c_str(),
              sub_port_);
  CHECK(evhttpcon_);
  evhttp_connection_set_closecb(evhttpcon_, ConnCloseCB, this);
  //evhttp_connection_set_retries(evhttpcon_, -1); // retry infinitely 
}

void SessionClient::CloseConn() {
  evhttp_connection_free(evhttpcon_);
}


void SessionClient::ConnCloseCB(struct evhttp_connection* conn, void *ctx) {
  VLOG(5) << "ConnCloseCB";
}

void SessionClient::SubDoneCB(struct evhttp_request *req, void *ctx) {
  VLOG(5) << "enter SubDoneCB";
  SessionClient* that = static_cast<SessionClient*>(ctx);
  CHECK(that);
  char buffer[FLAGS_read_buffer_size];
  int nread;

  if (req == NULL || evhttp_request_get_response_code(req) != 200) {
    int errcode = EVUTIL_SOCKET_ERROR();
    LOG(ERROR) << "socket error :" << evutil_socket_error_to_string(errcode);
    that->parent_->MakeCliErrEvent(new CliErrInfo(that->client_id_, "socket error", that->parent_));
    return;
  }

  while ((nread = evbuffer_remove(evhttp_request_get_input_buffer(req), buffer, sizeof(buffer))) > 0) {
    VLOG(5) << nread ;
    /* These are just arbitrary chunks of 256 bytes.
     * They are not lines, so we can't treat them as such. */
    fwrite(buffer, nread, 1, stdout);
  }

  VLOG(5) << "finished SubDoneCB";
}

void SessionClient::SubChunkCB(struct evhttp_request* req, void * ctx) {
  SessionClient* that = static_cast<SessionClient*>(ctx);
  VLOG(5) << "enter SubChunkCB";
  char buffer[FLAGS_read_buffer_size];
  int nread;
  string chunkdata;
  Json::Value value;
  Json::Reader reader;
  Json::FastWriter writer;
  string content2send;
  //TODO check parse

  //TODO when errno == EAGAIN
  while((nread = evbuffer_remove(evhttp_request_get_input_buffer(req), buffer, sizeof(buffer))) > 0) {
    VLOG(5) << "read buffer size: " << nread;
    VLOG(5) << "read buffer date: " << string(buffer, nread);
    //TODO
    if(!reader.parse(buffer, buffer + nread, value)) {
      LOG(ERROR) << "json parse failed, data" << string(buffer, nread);
      continue;
    }
    //value["content"];
    //VLOG(5) << value.toStyledString();
    string uri = string_format("/pub?cname=12&content=%s", value["type"].asCString());
    //VLOG(5) << uri;
    //that->parent_->MakePubEvent(0, uri.c_str());
  }
  VLOG(5) << "finished SubChunkCB";
}

void SessionClient::ReqErrorCB(enum evhttp_request_error err, void * ctx) {
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

void SessionClient::PubDoneCB(struct evhttp_request* req, void * ctx) {
  VLOG(5) << "PubDoneCB";
  SessionClient* that = static_cast<SessionClient*>(ctx);
  char buffer[FLAGS_read_buffer_size];
  int nread;

  if (req == NULL || evhttp_request_get_response_code(req) != 200) {
    int errcode = EVUTIL_SOCKET_ERROR();
    LOG(ERROR) << "socket error :" << evutil_socket_error_to_string(errcode);
    that->parent_->MakeCliErrEvent(new CliErrInfo(that->client_id_, "socket error", that->parent_));
    return;
  }

  while ((nread = evbuffer_remove(evhttp_request_get_input_buffer(req), buffer, sizeof(buffer))) > 0) {
    VLOG(5) << string(buffer, nread);
  }

}

void SessionClient::SubCompleteCB(struct evhttp_request* req, void * cxt) {
  VLOG(5) << "SubCompleteCB";
}

void SessionClient::PubCompleteCB(struct evhttp_request* req, void * ctx) {
  VLOG(5) << "PubCompleteCB";
}

} // namespace xcomet
