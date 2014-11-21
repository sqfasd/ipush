#ifndef XCOMET_ROUTER_SERVER_H
#define XCOMET_ROUTER_SERVER_H

#include <cstdlib>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include "deps/base/logging.h"
//#include <evhttp.h>
//#include <event2/event.h>
//#include <event2/listener.h>

namespace xcomet {

class RouterServer {
 public:
  static const size_t LISTEN_QUEUE_LEN = 1024;
  static const size_t MAX_EPOLL_SIZE = 1024;
  static const size_t SOCKET_READ_BUFFER_SIZE = 4096;
        
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
  void Response(int sockfd);
  void CloseSocket(int sockfd);
  inline static bool SetNonBlock(int sockfd) {
    int flags, s;
    flags = fcntl(sockfd, F_GETFD, 0);
    if(flags == -1) {
      LOG(ERROR) << strerror(errno);
      return false;
    }
    flags |= O_NONBLOCK;
    s = fcntl (sockfd, F_SETFL, flags);
    if (s == -1) {
      LOG(ERROR) << strerror(errno);
      return false;
    }
    return true;
  }
};

} // namespace xcomet

#endif
