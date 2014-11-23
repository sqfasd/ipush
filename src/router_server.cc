#include "router_server.h"

DEFINE_int32(listen_port, 8080, "");
DEFINE_int32(read_buffer_size,  16483, "");

namespace xcomet {

RouterServer::RouterServer() { 
  Init();
}

RouterServer::~RouterServer() {
}

void RouterServer::Init() {
  evbase_ = event_base_new();
  CHECK(evbase_);

  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = 0;
  sin.sin_port = htons(FLAGS_listen_port);

  listener_ = socket(AF_INET, SOCK_STREAM, 0);
  evutil_make_socket_nonblocking(listener_);

  CHECK(-1 != bind(listener_, (struct sockaddr*)&sin, sizeof(sin))) << strerror(errno); 

  CHECK(-1 != listen(listener_, 16)) << strerror(errno);


  listener_event_ = event_new(evbase_, listener_, EV_READ|EV_PERSIST, AcceptCB, static_cast<void*>(this));
  CHECK(-1 != event_add(listener_event_, NULL));

  LOG(INFO) << "start server, port:" << FLAGS_listen_port;
}

void RouterServer::Start() {

  event_base_dispatch(evbase_);
}

void RouterServer::AcceptCB(evutil_socket_t listener, short event, void *arg) {
  VLOG(5) << size_t(arg);
  //struct event_base *base = (struct event_base*)arg;
  RouterServer* that = static_cast<RouterServer*>(arg);
  CHECK(that);
  struct event_base *base = that->evbase_;
  CHECK(base);
  struct sockaddr_storage ss;
  socklen_t slen = sizeof(ss);
  int fd = accept(listener, (struct sockaddr*)&ss, &slen);
  if (fd < 0) {
    LOG(ERROR) << strerror(errno);
  } else if (fd > FD_SETSIZE) {
    LOG(ERROR) << "accept socket fd > FD_SETSIZE, close it.";
    close(fd);
  } else {
    VLOG(5) << "new a socket bufferevent";
    struct bufferevent *bev;
    evutil_make_socket_nonblocking(fd);
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, ReadCB, NULL, ErrorCB, NULL);
    bufferevent_setwatermark(bev, EV_READ, 0, FLAGS_read_buffer_size);
    bufferevent_enable(bev, EV_READ|EV_WRITE);
  }
}

void RouterServer::ReadCB(struct bufferevent* bev, void *ctx) {
  VLOG(5) << "enter ReadCB";
  struct evbuffer *input, *output;
  char buf[FLAGS_read_buffer_size];
  input = bufferevent_get_input(bev);
  output = bufferevent_get_output(bev);

  while (evbuffer_get_length(input)) {
    int n = evbuffer_remove(input, buf, sizeof(buf));
    VLOG(5) << "evbuffer_remove";
    evbuffer_add(output, buf, n);
  }
  // TODO bug when input's length equal to FLAGS_read_buffer_size
  evbuffer_add(output, "\n", 1);
  VLOG(5) << "finished ReadCB";
}

void RouterServer::ErrorCB(struct bufferevent* bev, short error, void *ctx) {
  if (error & BEV_EVENT_EOF) {
    /* connection has been closed, do any clean up here */
    /* ... */
    VLOG(5) << "connection has been closed from remote.";
  } else if (error & BEV_EVENT_ERROR) {
    /* check errno to see what error occurred */
    /* ... */
    LOG(ERROR) << strerror(EVUTIL_SOCKET_ERROR());
  } else if (error & BEV_EVENT_TIMEOUT) {
    /* must be a timeout event handle, handle it */
    /* ... */
    VLOG(5) << "timeout happened";
  }
  bufferevent_free(bev);
}

} // namespace xcomet

int main(int argc, char ** argv) {
  base::ParseCommandLineFlags(&argc, &argv, false);
  xcomet::RouterServer server;
  server.Start();
  return EXIT_SUCCESS;
}

