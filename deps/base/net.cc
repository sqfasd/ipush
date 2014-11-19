
#include "base/net.h"

#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include "base/yr.h"
#include "base/logging.h"
#include "base/time.h"

namespace base {

int SetReusable(int sock) {
  int on = 1;
  return setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
}

int SetNonblocking(int sock) {
  int opts;
  opts = fcntl(sock, F_GETFL);
  if (opts < 0) {
    return -1;
  }
  opts = opts | O_NONBLOCK;
  if (fcntl(sock, F_SETFL, opts) < 0) {
    return -1;
  }
  int on = 1;
  setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
  return 0;
}

int TcpListen(int port, int backlog) {
  int sock;
  const int on = 1;
  struct sockaddr_in sin;

  sock = socket(PF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    LOG(WARNING) << "create socket failed!";
    return -1;
  }
  memset(&sin, 0, sizeof(struct sockaddr_in));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port = htons(port);
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));

  if (bind(sock, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
    LOG(WARNING) << "tcp bind failed!";
    close(sock);
    return -1;
  }
  if (backlog <= 0) backlog = 5;

  if (listen(sock, backlog) < 0) {
    LOG(WARNING) << "tcp listen failed, "
                 << " port[" << port << "] "
                 << "backlog[" << backlog << "]";
    close(sock);
    return -1;
  }
  return sock;
}

int NetSelect(int nfds,
              fd_set *readfds,
              fd_set *writefds,
              fd_set *exceptfds,
              struct timeval *timeout) {
  int val;
  while(true) {
    val = select(nfds, readfds, writefds, exceptfds, timeout);
    if (val < 0) {
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
        continue;
      }
      LOG(WARNING) << "select failed, "
        << "error[" << errno << "] "
        << "msg[" << strerror(errno) << "]";
    }
    break;
  }
  if (val == 0) {
    errno = ETIMEDOUT;
  }
  return val;
}

int Connectoms(int sock,
               const struct sockaddr *psa,
               socklen_t socklen,
               int msecs) {
  if (sock < 0 || psa == NULL) return -1;
  int ret = 0;
  if (msecs <= 0) {
    ret = connect(sock, psa, socklen);
    if (ret == -1) {
      LOG(WARNING) << "connect failed, "
                   << "error[" << errno << "] "
                   << "msg[" << strerror(errno) << "]";
    }
    return ret;
  }

  int error = 0;
  socklen_t len;
  fd_set rset, wset;
  struct timeval tv;
  //  SetNonblocking(sock);
  int flags = fcntl(sock, F_GETFL, 0);
  fcntl(sock, F_SETFL, flags | O_NONBLOCK);

  ret = connect(sock, psa, socklen);
  if (ret == -1 && errno != EINPROGRESS) {
    LOG(WARNING) << "connect failed, "
                 << "error[" << errno << "] "
                 << "msg[" << strerror(errno) << "]";
    return -1;
  }
  if (ret != 0) {
    FD_ZERO(&rset);
    FD_SET(sock, &rset);
    wset = rset;
    tv.tv_sec = msecs / 1000;
    tv.tv_usec = msecs % 1000 * 1000;
    if (select(sock+1, &rset, &wset, NULL, &tv) == 0) {
      LOG(WARNING) << "select failed, "
                   << "error[" << ETIMEDOUT << "] "
                   << "msg[" << strerror(ETIMEDOUT) << "]";
      return -1;
    }
    if (FD_ISSET(sock, &rset) || FD_ISSET(sock, &wset)) {
      len = sizeof(error);
      if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) != 0) {
        LOG(WARNING) << "getsockopt failed, error "
                     << "[" << errno << "]"
                     << "msg[" << strerror(errno) << "]";
        return -1;
      }
    } else {
      LOG(WARNING) << "FD_ISSET failed";
      return -1;
    }
  }
  fcntl(sock, F_SETFL, flags);
  if (error) {
    return -1;
  }
  return 0;
}

int TcpConnect(const char*host, int port, int msecs) {
  int sock;
  struct sockaddr_in sin;
  char buf[8192];
  int on = 1;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    LOG(WARNING) << "create socket failed, "
                 << "error[" << errno << "] "
                 << "msg[" << strerror(errno) << "]";
    return -1;
  }
  memset(&sin, 0, sizeof(struct sockaddr_in));
  sin.sin_addr.s_addr = inet_addr(host);
  if (sin.sin_addr.s_addr == INADDR_NONE) {
    struct hostent he, *p;
    int err, ret;
    ret = gethostbyname_r(host, &he, buf, sizeof(buf), &p, &err);
    if ( ret < 0 ) {
      LOG(WARNING) << "gethostbyname_r failed";
      close(sock);
      return -1;
    }
    memcpy(&sin.sin_addr.s_addr, he.h_addr, sizeof(sin.sin_addr.s_addr));
  }
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  if (Connectoms(sock, (struct sockaddr*)&sin, sizeof(sin), msecs) < 0) {
    //    LOG(WARNING) << "Connect failed";
    close(sock);
    return -1;
  }
  setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
  return sock;
}

int TcpAccept(int sock, struct sockaddr *addr, socklen_t *addrlen) {
  int confd;
  while ((confd = accept(sock, addr, addrlen)) == -1) {
#ifdef EPROTO
    if ( errno != EPROTO && errno != ECONNABORTED ) {
#else
      if ( errno != ECONNABORTED ) {
#endif
        LOG(WARNING) << "accept failed, error "
                     << "[" << errno << "] "
                     << "msg[" << strerror(errno) << "]";
        return -1;
      }
  }
  int on = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
  // add for close_wait
//  linger m_sLinger;
//  m_sLinger.l_onoff = 1;
//  m_sLinger.l_linger = 0;
//  setsockopt(
//      sock, SOL_SOCKET, SO_LINGER, (const char*)&m_sLinger, sizeof(linger));

//  int sock_buf_size = 1024*1024*10;
//  int ret = setsockopt(
//      confd, SOL_SOCKET, SO_SNDBUF,
//        (char*)&sock_buf_size, sizeof(sock_buf_size));
//  if (ret < 0) {
//    LOG(WARNING) << "set sock buf size failed";
//  }
  // add over
  return confd;
}

bool TcpPending(int sock, sockpend_t pending, int timeout) {
  int status = 0;
  struct timeval tv, *tvp;
  fd_set fds;

  if (timeout == TIMEOUT_INF) {
    tvp = NULL;
  } else {
    tvp = &tv;
    tvp->tv_sec = timeout / 1000;
    tvp->tv_usec = (timeout % 1000) * 1000;
  }
  FD_ZERO(&fds);
  FD_SET(sock, &fds);
  switch (pending) {
    case SOCKET_PENDING_INPUT:
      status = select(sock+1, &fds, NULL, NULL, tvp);
      break;
    case SOCKET_PENDING_OUTPUT:
      status = select(sock+1, NULL, &fds, NULL, tvp);
      break;
    case SOCKET_PENDING_ERROR:
      status = select(sock+1, NULL, NULL, &fds, tvp);
      break;
  }
  if (status < 1) return false;
  if (FD_ISSET(sock, &fds)) return true;

  return false;
}

int TcpRecv(int sock, void *buf, int buf_size, int timeout) {
  int read_num;
  int64_t start_tm = GetTimeInMs();
  while (true) {
    int64_t now = GetTimeInMs();
    if (start_tm + timeout < now) {
      LOG(WARNING) << "Recv data timeout, "
                   << "sock[" << sock << "] "
                   << "timeout[" << timeout<< "]";
      return RECV_TIMEOUT;
    }
    read_num = read(sock, buf, buf_size);
    if (read_num >= 0) {
      break;
    }
    if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
      continue;
    } else {
      LOG(WARNING) << "Fail to recv data from sock[" << sock << "]"
        << "error[" << errno << "]"
        << "msg[" << strerror(errno) << "]";
      return RECV_DATA_ERROR;
    }
  }
  return read_num;
}

int TcpRecvLen(int sock, void *buf, int size, int timeout) {
  int left_bytes = size;
  int read_num;
  char *pbuf = reinterpret_cast<char *>(buf);
  char *psrc = pbuf;

  int64_t start_tm = GetTimeInMs();
  while (left_bytes > 0) {
    int64_t now = GetTimeInMs();
    if (start_tm + timeout < now) {
      LOG(WARNING) << "Recv data timeout, sock "
                   << "[" << sock << "]"
                   << "timeout[" << timeout<< "]";
      return RECV_TIMEOUT;
    }
    read_num = read(sock, pbuf, left_bytes);
    if (read_num <= 0) {
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
        continue;
      } else {
        LOG(WARNING) << "Fail to recv data from sock[" << sock << "]"
          << "error[" << errno << "]"
          << "msg[" << strerror(errno) << "]";
        return RECV_DATA_ERROR;
      }
    }
    left_bytes -= read_num;
    pbuf += read_num;
  }
  return pbuf - psrc;
}

int TcpSend(int sock, const void *buffer, int size) {
  int left_bytes = size;
  int read_num;
  if (buffer == NULL || size <= 0) return 0;

  const char *pbuf = reinterpret_cast<const char *>(buffer);
  while (left_bytes > 0) {
    read_num = write(sock, pbuf, left_bytes);
    if (read_num == -1) {
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
        usleep(5);
        continue;
      } else {
        LOG(WARNING) << "Fail to send data to sock[" << sock << "]"
          << "error[" << errno << "]"
          << "msg[" << strerror(errno) << "]";
        return -1;
      }
    }
    left_bytes -= read_num;
    pbuf += read_num;
    VLOG(9) << "rd" << read_num << " left_types:" <<  left_bytes;
  }
  return size;
}

}  //  namespace base
