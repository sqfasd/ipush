#include "session_client.h"

#include "deps/jsoncpp/include/json/json.h"
#include "deps/base/flags.h"
#include "deps/base/logging.h"
#include "router_server.h"
#include "deps/base/time.h"

DEFINE_int32(read_buffer_size,  4096, "");
DEFINE_int32(retry_interval, 2000, "milliseconds");

namespace xcomet {

SessionClient::SessionClient(
            struct event_base* evbase, 
            const string& host,
            int port, 
            const string& uri)
    : evbase_(evbase), host_(host), port_(port), uri_(uri) {
  InitConn();
}

SessionClient::~SessionClient() {
  CloseConn();
}

//memory leak ? evhttpcon_
void SessionClient::MakeRequestEvent() {
  VLOG(5) << "enter MakeRequestEvent";
  struct evhttp_request* req = evhttp_request_new(SubReqDoneCB, this);
  evhttp_request_set_chunked_cb(req, SubReqChunkCB);
  evhttp_request_set_error_cb(req, ReqErrorCB);
  int r = evhttp_make_request(evhttpcon_, req, EVHTTP_REQ_GET, uri_.c_str());
  CHECK(r == 0);
}

void SessionClient::InitConn() {
  //struct bufferevent* bev = bufferevent_socket_new(evbase_, -1, 0);
  VLOG(5) << "init connections , host: " 
          << host_ 
          << " port: " 
          << port_;
  evhttpcon_ = evhttp_connection_base_bufferevent_new(
              evbase_, 
              NULL, 
              NULL, 
              host_.c_str(),
              port_);
  //evhttpcon_ = evhttp_connection_base_bufferevent_new(evbase_, NULL, bev, FLAGS_sserver_sub_ip.c_str(), FLAGS_sserver_sub_port);
  //evhttpcon_ = evhttp_connection_new(FLAGS_sserver_sub_ip.c_str(), FLAGS_sserver_sub_port);
  //evhttpcon_ = evhttp_connection_base_new(evbase_, NULL, FLAGS_sserver_sub_ip.c_str(), FLAGS_sserver_sub_port);
  CHECK(evhttpcon_);
  evhttp_connection_set_closecb(evhttpcon_, ConnCloseCB, this);
  //evhttp_connection_set_retries(evhttpcon_, -1); // retry infinitely TODO
}

void SessionClient::CloseConn() {
  evhttp_connection_free(evhttpcon_);
}


void SessionClient::ConnCloseCB(struct evhttp_connection* conn, void *ctx) {
  VLOG(5) << "enter ConnCloseCB";
  SessionClient* that = static_cast<SessionClient*>(ctx);
  VLOG(5) << "sleep " << FLAGS_retry_interval;
  base::MilliSleep(FLAGS_retry_interval); //TODO
  that->MakeRequestEvent(); // reopen it
  VLOG(5) << "finished ConnCloseCB";
}

//TODO to reconnect
//TODO error handler
void SessionClient::SubReqDoneCB(struct evhttp_request *req, void *ctx) {
  VLOG(5) << "enter SubReqDoneCB";
  SessionClient* that = static_cast<SessionClient*>(ctx);
  CHECK(that);
  char buffer[FLAGS_read_buffer_size];
  int nread;

  if (req == NULL || evhttp_request_get_response_code(req) != 200) {
    /* If req is NULL, it means an error occurred, but
     * sadly we are mostly left guessing what the error
     * might have been.  We'll do our best... */
    //struct bufferevent *bev = (struct bufferevent *) ctx;
    int errcode = EVUTIL_SOCKET_ERROR();
    LOG(ERROR) << "socket error :" << evutil_socket_error_to_string(errcode);
    VLOG(5) << "sleep " << FLAGS_retry_interval;
    base::MilliSleep(FLAGS_retry_interval); //TODO
    //that->MakeRequestEvent(req->); //TODO // retry;
    return;
  }

  //int code = evhttp_request_get_response_code(req);
  //if (code != 200) {
  //  LOG(ERROR) << "error happend.";
  //  return;
  //}

  while ((nread = evbuffer_remove(evhttp_request_get_input_buffer(req), buffer, sizeof(buffer))) > 0) {
    VLOG(5) << nread ;
    /* These are just arbitrary chunks of 256 bytes.
     * They are not lines, so we can't treat them as such. */
    fwrite(buffer, nread, 1, stdout);
  }

  VLOG(5) << "finished SubReqDoneCB";
}

void SessionClient::SubReqChunkCB(struct evhttp_request* req, void * ctx) {
  VLOG(5) << "enter SubReqDoneCB";
  SessionClient* that = static_cast<SessionClient*>(ctx);
  VLOG(5) << "enter SubReqChunkCB";
  char buffer[FLAGS_read_buffer_size];
  int nread;
  string chunkdata;
  Json::Value value;
  Json::Reader reader;
  //TODO check parse

  //TODO when errno == EAGAIN
  while((nread = evbuffer_remove(evhttp_request_get_input_buffer(req), buffer, sizeof(buffer))) > 0) {
    VLOG(5) << "read buffer size: " << nread;
    VLOG(5) << "read buffer date: " << string(buffer, nread);
    //fwrite(buffer, nread, 1, stdout);
    //TODO
    if(!reader.parse(buffer, buffer + nread, value)) {
      LOG(ERROR) << "json parse failed, data" << string(buffer, nread);
      continue;
    }
    //that->MakePubReq();
    VLOG(5) << value.toStyledString();
  }
  VLOG(5) << "finished SubReqChunkCB";
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

void SessionClient::PubReqDoneCB(struct evhttp_request* req, void * ctx) {
  VLOG(5) << "enter PubReqDoneCB";
  char buffer[FLAGS_read_buffer_size];
  int nread;

  if (req == NULL) {
    //TODO
    LOG(ERROR) << "error";
    return;
  }

  while ((nread = evbuffer_remove(evhttp_request_get_input_buffer(req), buffer, sizeof(buffer))) > 0) {
    VLOG(5) << string(buffer, nread);
  }
}

}
