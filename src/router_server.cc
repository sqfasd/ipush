#include "router_server.h"
#include "deps/base/time.h"

DEFINE_string(sserver_sub_ip, "master.domain.com", "");
DEFINE_int32(sserver_sub_port, 8100, "");
DEFINE_string(sserver_sub_uri, "/stream?cname=12", "");
//DEFINE_int32(listen_port, 8080, "");
DEFINE_int32(read_buffer_size,  4096, "");
DEFINE_int32(retry_interval, 2000, "2000 milliseconds");
DEFINE_bool(libevent_debug_log, false, "for debug logging");

namespace xcomet {

RouterServer::RouterServer() { 
  evbase_ = event_base_new();
  CHECK(evbase_);
}

RouterServer::~RouterServer() {
  event_base_free(evbase_);
}

void RouterServer::OpenConn() {
  //bev_ = bufferevent_socket_new(evbase_, -1, BEV_OPT_CLOSE_ON_FREE);
  //bev_ = bufferevent_socket_new(evbase_, -1, 0);
  //CHECK(bev_);
  evhttpcon_ = evhttp_connection_base_bufferevent_new(evbase_, NULL, NULL, FLAGS_sserver_sub_ip.c_str(), FLAGS_sserver_sub_port);
  //evhttpcon_ = evhttp_connection_base_new(evbase_, NULL, FLAGS_sserver_sub_ip.c_str(), FLAGS_sserver_sub_port);
  CHECK(evhttpcon_);
  // retry infinitely
  //evhttp_connection_set_retries(evhttpcon_, -1);
  struct evhttp_request* req = evhttp_request_new(SubReqDoneCB, this);
  evhttp_request_set_chunked_cb(req, SubReqChunkCB);
  evhttp_request_set_error_cb(req, ReqErrorCB);
  int r = evhttp_make_request(evhttpcon_, req, EVHTTP_REQ_GET, FLAGS_sserver_sub_uri.c_str());
  CHECK(r == 0);
}

void RouterServer::CloseConn() {
  evhttp_connection_free(evhttpcon_);
}

void RouterServer::Start() {
  LOG(INFO) << "open connection";
  OpenConn();
  event_base_dispatch(evbase_);
  // TODO retry
  //CloseConn();
  //LOG(ERROR) << "disconnected ... , sleep millisleep " << FLAGS_retry_interval << ", retry it .";
  //base::MilliSleep(FLAGS_retry_interval);
}


//TODO to reconnect
//TODO error handler
void RouterServer::SubReqDoneCB(struct evhttp_request *req, void *ctx) {
  VLOG(5) << "enter SubReqDoneCB";
  RouterServer* that = static_cast<RouterServer*>(ctx);
  CHECK(that);
  char buffer[FLAGS_read_buffer_size];
  int nread;

  if (req == NULL) {
    /* If req is NULL, it means an error occurred, but
     * sadly we are mostly left guessing what the error
     * might have been.  We'll do our best... */
    //struct bufferevent *bev = (struct bufferevent *) ctx;
    int errcode = EVUTIL_SOCKET_ERROR();
    LOG(ERROR) << "socket error :" << evutil_socket_error_to_string(errcode);
    that->CloseConn();
    base::MilliSleep(FLAGS_retry_interval);
    that->OpenConn();
    return;
  }

  int code = evhttp_request_get_response_code(req);
  if (code != 200) {
    that->CloseConn();
    base::MilliSleep(FLAGS_retry_interval);
    that->OpenConn();
    LOG(ERROR) << "error happend.";
    return;
  }

  while ((nread = evbuffer_remove(evhttp_request_get_input_buffer(req), buffer, sizeof(buffer))) > 0) {
    VLOG(5) << nread ;
    /* These are just arbitrary chunks of 256 bytes.
     * They are not lines, so we can't treat them as such. */
    fwrite(buffer, nread, 1, stdout);
  }

  VLOG(5) << "finished SubReqDoneCB";
}

void RouterServer::SubReqChunkCB(struct evhttp_request* req, void * ctx) {
  VLOG(5) << "enter SubReqDoneCB";
  RouterServer* that = static_cast<RouterServer*>(ctx);
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

void RouterServer::ReqErrorCB(enum evhttp_request_error err, void * ctx) {
  LOG(ERROR) << "ReqError";
  //switch err {
  //case EVREQ_HTTP_TIMEOUT:
  //EVREQ_HTTP_EOF,
  //EVREQ_HTTP_INVALID_HEADER,
  //EVREQ_HTTP_BUFFER_ERROR,
  //EVREQ_HTTP_REQUEST_CANCEL,
  //EVREQ_HTTP_DATA_TOO_LONG 
}

void RouterServer::PubReqDoneCB(struct evhttp_request* req, void * ctx) {
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

//TODO to close connection
void RouterServer::MakePubReq(const char* host, const int port, const char* uri) {
  //struct bufferevent * bev = bufferevent_socket_new(evbase_, -1, BEV_OPT_CLOSE_ON_FREE);
  //CHECK(bev);
  //const char * host = "slave1.domain.com";
  //const int port = 9101;
  struct evhttp_connection * evhttpcon = evhttp_connection_base_bufferevent_new(evbase_, NULL, NULL, host, port);
  CHECK(evhttpcon);
  struct evhttp_request* req = evhttp_request_new(PubReqDoneCB, NULL);
  struct evkeyvalq * output_headers;
  struct evbuffer * output_buffer;
  output_headers = evhttp_request_get_output_headers(req);
  evhttp_add_header(output_headers, "Host", host);
  evhttp_add_header(output_headers, "Connection", "close");
  int r = evhttp_make_request(evhttpcon, req, EVHTTP_REQ_GET, uri);
  CHECK(r == 0);
  VLOG(5) << "MakePubReq finished.";
}

} // namespace xcomet

int main(int argc, char ** argv) {
  base::ParseCommandLineFlags(&argc, &argv, false);
  if(FLAGS_libevent_debug_log) {
    event_enable_debug_logging(EVENT_DBG_ALL);
  }
  xcomet::RouterServer server;
  server.Start();
  return EXIT_SUCCESS;
}

