
#ifndef BASE_NET_H_
#define BASE_NET_H_

#include <sys/socket.h>

namespace base {

#define TIMEOUT_INF 0

typedef enum {
  SOCKET_PENDING_INPUT,
  SOCKET_PENDING_OUTPUT,
  SOCKET_PENDING_ERROR
}sockpend_t;

typedef enum {
  RECV_TIMEOUT = -1,
  RECV_DATA_ERROR = -2
}net_err_t;

// 将socket设置为非阻塞模式
int SetNonblocking(int sock);

// 将socket设置为可重用模式,通常用于解决TIME_WAIT端口导致的无法重启问题
// 由于TcpAccept和TcpConnect均默认调用了这个函数，
// 因此用户一般不用显式调用
int SetReusable(int sock);

// safe select wrapper
int NetSelect(int nfds,
              fd_set *readfds,
              fd_set * writefds,
              fd_set *exceptfds,
              struct timeval *timeout);

// 由static设置到开发接口
int Connectoms(int sock,
               const struct sockaddr *psa,
               socklen_t socklen,
               int msecs);

// safe listen wrapper, 第二个参数为等待连接队列的长度
int TcpListen(int port, int backlog);

// safe connect wrapper
// 第三个参数为connect的等待时间，通常设置100ms即可
int TcpConnect(const char *host, int port, int msecs);

// safe accept wrapper, 后面两个参数通常不用，可置NULL
int TcpAccept(int sock, struct sockaddr *addr, socklen_t *addrlen);

// 由static设置到开发接口
bool TcpPending(int sock, sockpend_t pending, int timeout);

//  从sock读一次不定长数据
//  适用于读取未知大小数据包的情况，如读取http请求或FIN包
//  ，成功读取一次数据后即返回
//  参数：buf_size 存放数据的缓冲区长度
//  结束条件:
//  1,成功读到一次数据,返回读到的字节数，注意，如果是FIN包，返回的是0
//  2,超时, 返回RECV_TIMEOUT
//  3,读取错误, 返回RECV_DATA_ERROR
int TcpRecv(int sock, void *buf, int buf_size, int timeout);

// 读取长度为size的内容
// 适用于读取已知大小数据包的情况，函数会一直等待，直到读满数据和超时为止
// 结束条件:
// 1,读满, 返回size
// 2,超时, 返回RECV_TIMEOUT
// 3,读取错误, 返回RECV_DATA_ERROR
int TcpRecvLen(int sock, void *buf, int size, int timeout);

// 发送长度为size的内容
// 结束条件：
// 1,发送完毕，返回size
// 2,发送失败，返回-1
int TcpSend(int sock, const void *buffer, int size);

}  // namespace base

#endif  // BASE_NET_H_
