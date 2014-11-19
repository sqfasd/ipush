#ifndef XCOMET_ROUTER_SERVER_H
#define XCOMET_ROUTER_SERVER_H

#include <cstdlib>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
//#include <evhttp.h>
//#include <event2/event.h>
//#include <event2/listener.h>

namespace xcomet {

class RouterServer {
 public:
  static const size_t LISTEN_QUEUE_LEN = 1024;
  static const size_t MAX_EPOLL_SIZE = 1024;
        
 public:
  RouterServer();
  ~RouterServer();
 public:
  void Start();
 private:
  void InitHostSocket(size_t port);
  void InitEpoll();
  bool EpollAdd(int sockfd, uint32_t events);
 private:
  int host_socket_;
  int epoll_fd_;
  size_t epoll_size_;
 private:
  inline static bool SetNonBlock(int sockfd) {
    return -1 != fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK);
  }
  //struct event_base *evbase_;
  //struct evhttp *admin_http_;
};

} // namespace xcomet

#endif
