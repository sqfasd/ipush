#include "router_server.h"
#include "deps/base/logging.h"
#include "deps/base/flags.h"

namespace xcomet {

//void LookupPresenceHandler(struct evhttp_request *req, void *arg) {
//  LOG(INFO) << "LookupPresenceHandler";
//}

DEFINE_int32(listen_port, 9095, "the listen port for router server");


//RouterServer::RouterServer() {
  
  //evbase_ = event_base_new();
  //CHECK(evbase_ != NULL);
  //admin_http_ = evhttp_new(evbase_);
  //CHECK(admin_http_ != NULL);

  //evhttp_set_cb(admin_http_, "/presence", LookupPresenceHandler, NULL);

  //struct evhttp_bound_socket *handle;
  //handle = evhttp_bind_socket_with_handle(front_http, front_ip.c_str(), port);
//}

RouterServer::RouterServer() {
  InitHostSocket(FLAGS_listen_port);
  InitEpoll();
}

RouterServer::~RouterServer() {
}
  //event_base_free(evbase_);
  //evhttp_free(admin_http_);

void RouterServer::Start() {
  sockaddr_in clientaddr;
  socklen_t socksize = sizoef(clientaddr);
  struct epoll_event events[MAX_EPOLL_SIZE];
  int nfds, clientsock;
  while(true) {
    nfds = epoll_wait(epoll_fd_, events, epoll_size_, -1);
    if(-1 == nfds) {
      LOG(ERROR) << strerror(errno);
      continue;
    }
    for(size_t i = 0; i < nfds; i++) {
      if(events[i].data.fd == host_socket_) {
        clientsock = accept(host_socket_, (struct sockaddr*) &clientaddr, &nSize);
        if(-1 == clientsock) {
          LOG(ERROR) << strerror(errno);
          continue;
        }
        if(!EpollAdd(clientsock, EPOLLIN | EPOLLET)) {
          LOG(ERROR) << "EpollAdd Failed!";
          CloseSocket(sockfd);
          continue;
        }
      } else {
          //TODO
        Response(events[i].data.fd);
      }
    }
  }
}

void RouterServer::CloseSocket(int sockfd) {
  if(-1 == close(sockfd)) {
    LOG(ERROR) << strerror(errno);
  }
  _epollSize --;
}

void RouterServer::InitHostSocket(size_t port) {
  int code;
  host_socket_ = socket(AF_INET, SOCK_STREAM, 0);
  CHECK(host_socket_ != -1);

  //TODO
  int nRet = -1;
  code = setsockopt(host_socket_, SOL_SOCKET, SO_REUSEADDR, (char*)&nRet, sizeof(nRet));
  CHECK(code != -1);
  
  struct sockaddr_in addrSock;
  addrSock.sin_family = AF_INET;
  addrSock.sin_port = htons(port);
  addrSock.sin_addr.s_addr = htonl(INADDR_ANY);
  
  code = ::bind(host_socket_, (sockaddr*)&addrSock, sizeof(sockaddr));
  CHECK(code != -1);

  code = listen(host_socket_, LISTEN_QUEUE_LEN);
  CHECK(code != -1);
}

void RouterServer::InitEpoll() {
  epoll_fd_ = epoll_create(MAX_EPOLL_SIZE);
  CHECK(epoll_fd_ != -1);

  CHECK(EpollAdd(host_socket_, EPOLLIN));
}

bool RouterServer::EpollAdd(int sockfd, uint32_t events) {
  if(!SetNonBlock(sockfd)) {
    LOG(ERROR) << "setnonblock failed.";
    return false;
  }
  struct epoll_event ev;
  ev.data.fd = sockfd;
  ev.events = events;
  if(epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
    LOG(ERROR) << "insert socket " << sockfd << " into epoll failed: " << strerror(errno);
    return false;
  }
  epoll_size_ ++;
  return true;
}

} //namespace xcomet

int main(int argc, char ** argv) {
    base::ParseCommandLineFlags(&argc, &argv, false);
    xcomet::RouterServer server;
    server.Start();
    return EXIT_SUCCESS;
}
