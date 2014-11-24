#include "router_server.h"

DEFINE_string(sserver_sub_ip, "master.domain.com", "");
DEFINE_int32(sserver_sub_port, 8100, "");
DEFINE_string(sserver_sub_uri, "/stream?cname=12", "");
DEFINE_int32(listen_port, 8080, "");
DEFINE_int32(read_buffer_size,  4096, "");

namespace xcomet {

RouterServer::RouterServer() { 
  Init();
}

RouterServer::~RouterServer() {
  event_base_free(evbase_);
}

void RouterServer::Init() {
  evbase_ = event_base_new();
  CHECK(evbase_);
  bev_ = bufferevent_socket_new(evbase_, -1, BEV_OPT_CLOSE_ON_FREE);
  CHECK(bev_);
  evhttpcon_ = evhttp_connection_base_bufferevent_new(evbase_, NULL, bev_, FLAGS_sserver_sub_ip.c_str(), FLAGS_sserver_sub_port);
  assert(evhttpcon_);
  struct evhttp_request* req = evhttp_request_new(SubReqDoneCB, bev_);
  evhttp_request_set_chunked_cb(req, SubReqChunkCB);
  evhttp_request_set_error_cb(req, ReqErrorCB);
  int r = evhttp_make_request(evhttpcon_, req, EVHTTP_REQ_GET, FLAGS_sserver_sub_uri.c_str());
  CHECK(r == 0);
}

void RouterServer::Start() {
  event_base_dispatch(evbase_);
}


//TODO to reconnect
//TODO error handler
void RouterServer::SubReqDoneCB(struct evhttp_request *req, void *ctx) {
  VLOG(5) << "enter SubReqDoneCB";
  char buffer[256];
  int nread;

  if (req == NULL) {
    /* If req is NULL, it means an error occurred, but
     * sadly we are mostly left guessing what the error
     * might have been.  We'll do our best... */
    //struct bufferevent *bev = (struct bufferevent *) ctx;
    int errcode = EVUTIL_SOCKET_ERROR();
    LOG(ERROR) << "socket error :" << evutil_socket_error_to_string(errcode);
    return;
  } else {
    int errcode = EVUTIL_SOCKET_ERROR();
    LOG(ERROR) << "socket error :" << evutil_socket_error_to_string(errcode);
  }

  int code = evhttp_request_get_response_code(req);
  if (code == 200) {
    while ((nread = evbuffer_remove(evhttp_request_get_input_buffer(req), buffer, sizeof(buffer))) > 0) {
      VLOG(5) << nread ;
      /* These are just arbitrary chunks of 256 bytes.
       * They are not lines, so we can't treat them as such. */
      fwrite(buffer, nread, 1, stdout);
    }
  } else {
    LOG(ERROR) << "error happend.";
    //TODO
    // code may be 0 , so get code_line will error;
    //LOG(INFO) << "Response " << evhttp_request_get_response_code(req) << " " << evhttp_request_get_response_code_line(req);
  }

  VLOG(5) << "finished SubReqDoneCB";
}

void RouterServer::SubReqChunkCB(struct evhttp_request* req, void * arg) {
  VLOG(5) << "enter SubReqChunkCB";
  char buffer[256];
  int nread;
  //TODO when errno == EAGAIN
  while((nread = evbuffer_remove(evhttp_request_get_input_buffer(req), buffer, sizeof(buffer))) > 0) {
      fwrite(buffer, nread, 1, stdout);
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

} // namespace xcomet

int main(int argc, char ** argv) {
  base::ParseCommandLineFlags(&argc, &argv, false);
  xcomet::RouterServer server;
  server.Start();
  return EXIT_SUCCESS;
}

