#include "src/socketclient.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "deps/base/logging.h"
#include "deps/base/callback.h"
#include "src/utils.h"

using std::string;

//const int DEFAULT_KEEPALIVE_INTERVAL_SEC = 28 * 60L;
const int DEFAULT_KEEPALIVE_INTERVAL_SEC = 30L;

#define CERROR(msg) string(msg) + ": " + ::strerror(errno)

namespace xcomet {

BufferReader::BufferReader() {
  ::memset(buf_, 0, sizeof(buf_));
  start_ = 0;
  end_ = 0;
}

BufferReader::~BufferReader() {
}

const int ERROR_NOMORE_DATA = -100;

int BufferReader::Read(int fd) {
  if (Size() < 10 || end_ == sizeof(buf_)) {
    Shrink();
  }
  int ret = ::read(fd, buf_+end_, sizeof(buf_)-end_);
  VLOG(7) << "Buffer read n=" << ret;
  if (ret > 0) {
    end_ += ret;
  }
  return ret;
}

int BufferReader::Read(int fd, char* addr, int len) {
  VLOG(7) << "Buffer size=" << Size();
  if (Size() < len) {
    int ret = Read(fd);
    if (ret == 0) {
      return ret;
    }
  }
  int i = 0;
  int size = Size();
  for (; i < len && i < size; ++i) {
    addr[i] = buf_[start_++];
  }
  if (i == 0) {
    return ERROR_NOMORE_DATA;
  } else {
    return i;
  }
}

int BufferReader::ReadLine(int fd, char* addr) {
  int pos = FindCRLF();
  if (pos == -1) {
    int ret = Read(fd);
    if (ret <= 0) {
      return ret;
    } else {
      pos = FindCRLF();
      if (pos == -1) {
        return ERROR_NOMORE_DATA;
      }
    }
  }
  VLOG(7) << "in ReadLine crlf pos=" << pos;
  VLOG(7) << "in ReadLine start=" << start_ << ", end=" << end_;
  int i = 0;
  for (; start_ < pos + 2; ++i,++start_) {
    addr[i] = buf_[start_];
  }
  CHECK(i > 0);
  return i;
}

int BufferReader::FindCRLF() {
  for (int i = start_; i+1 < end_; ++i) {
    if (buf_[i] == '\r' && buf_[i+1] == '\n') {
      return i;
    }
  }
  return -1;
}

void BufferReader::Shrink() {
  if (start_ == 0) {
    return;
  }
  if (start_ == end_) {
    start_ = 0;
    end_ = 0;
    return;
  }
  if (end_ > start_) {
    int size = Size();
    ::memmove(buf_, buf_+start_, end_-start_);
    start_ = 0;
    end_ = start_ + size;
  }
}

Packet::Packet()
    : rstate_(RS_HEADER),
      len_(0),
      left_(0), 
      state_(NONE), 
      buf_start_(0) {
  ::memset(data_len_buf_, 0, sizeof(data_len_buf_));
}

Packet::~Packet() {
}

int Packet::Read(int fd) {
  while (true) {
    if (rstate_ == RS_HEADER) {
      char buf[MAX_DATA_LEN] = {0};
      int ret = reader_.ReadLine(fd, buf);
      if(ret > 0) {
        VLOG(7) << "ReadLine: " << buf;
        len_ = ::strtol(buf, NULL, 16) + 2;
        VLOG(7) << "Read len: " << len_;
        CHECK(len_ > 0 && len_ < 8000 * 1000);  // packet limit 8M
        left_ = len_;
        content_.resize(len_);
        rstate_ = RS_BODY;
      } else {
        return ret;
      }
    } else if (rstate_ == RS_BODY) {
      char* pcontent = const_cast<char*>(content_.c_str()) + len_ - left_;
      while (left_ > 0) {
        int n = reader_.Read(fd, pcontent, left_);
        VLOG(7) << "Read n=" << n << ", left=" << left_;
        if (n <= 0) {
          return n;
        } else {
          left_ -= n;
        }
      }
      int crlf_pos = content_.find("\r\n");
      if (crlf_pos != -1) {
        content_[crlf_pos] = 0;
      }
      return 1;
    }
  }
  LOG(ERROR) << "should not come here";
  return 0;
}

int Packet::Write(int fd) {
  while (true) {
    if (state_ == NONE) {
      ::memset(data_len_buf_, 0, sizeof(data_len_buf_));
      int n = ::snprintf(data_len_buf_,
                         MAX_DATA_LEN,
                         "%x\r\n",
                         (uint32)len_-2);
      CHECK(n < MAX_DATA_LEN);
      state_ = DATA_LEN;
      left_ = n;
      buf_start_ = 0;
      continue;
    } else if (state_ == DATA_LEN) {
      int ret = ::write(fd, data_len_buf_ + buf_start_, left_);
      if (ret < left_) {
        left_ = left_ - ret;
        buf_start_ = buf_start_ + ret;
        return ret;
      } else {
        left_ = len_;
        buf_start_ = 0;
        state_ = DATA_BODY;
        continue;
      }
    } else if (state_ == DATA_BODY) {
      int ret = ::write(fd, content_.c_str() + buf_start_, left_);
      if (ret < left_) {
        left_ = left_ - ret;
        buf_start_ = buf_start_ + ret;
        return ret;
      } else {
        return len_;
      }
    }
  }
  LOG(ERROR) << "should not come here";
  return 0;
}

SocketClient::SocketClient(const ClientOption& option)
    : sock_fd_(-1),
      option_(option),
      is_connected_(false),
      current_read_packet_(new Packet()),
      keepalive_interval_sec_(DEFAULT_KEEPALIVE_INTERVAL_SEC) {
  CHECK(::socketpair(AF_UNIX, SOCK_STREAM, 0, pipe_) == 0);
  SetNonblock(pipe_[1]);
}

SocketClient::~SocketClient() {
  WaitForClose();
  ::close(pipe_[0]);
  ::close(pipe_[1]);
}

int SocketClient::Connect() {
  is_connected_ = false;
  if (worker_thread_.get() != NULL) {
    worker_thread_->Join();
  }
  if (option_.host.empty() ||
      option_.port <= 0 ||
      option_.username.empty() ||
      option_.password.empty()) {
    LOG(ERROR) << "invalid client option";
    return -1;
  }

  sock_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd_== -1) {
    LOG(ERROR) << CERROR("socket error");
    return -3;
  }

  char buffer[1024] = {0};
  struct sockaddr_in server_addr;
  ::memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = ::inet_addr(option_.host.c_str());
  server_addr.sin_port = htons(option_.port);

  if (::connect(sock_fd_,
                (struct sockaddr*)&server_addr,
                sizeof(struct sockaddr)) == -1) {
    // TODO(qingfeng) check different errno to process
    LOG(ERROR) << CERROR("connect error");
    return -4;
  }
  int size = ::snprintf(
      buffer,
      sizeof(buffer),
      "GET /rsub?uid=%s&password=%s HTTP/1.1\r\n"
      "User-Agent: mobile_socket_client/0.1.0\r\n"
      "Accept: */*\r\n"
      "\r\n",
      option_.username.c_str(),
      option_.password.c_str());
  if (::send(sock_fd_, buffer, size, 0) < 0) {
    LOG(ERROR) << CERROR("send error");
    return -5;
  }
  // TODO(qingfeng) handle message after header
  ::memset(buffer, 0, sizeof(buffer));
  if (::recv(sock_fd_, buffer, sizeof(buffer), 0) < 0) {
    LOG(ERROR) << CERROR("receive error");
    return -6;
  }
  if (::strstr(buffer, "HTTP/1.1 200") == NULL) {
    LOG(INFO) << (string("connect failed: ") + buffer);
    return -7;
  }

  SetNonblock(sock_fd_);
  
  worker_thread_.reset(
      new base::ThreadRunner(
          base::NewOneTimeCallback(this, &SocketClient::Loop)));
  worker_thread_->Start();
  return 0;
}

void SocketClient::Loop() {
  is_connected_ = true;
  if (connect_cb_) {
    connect_cb_();
  }
  while (is_connected_) {
    struct pollfd pfds[2];
    pfds[0].fd = sock_fd_;
    pfds[0].revents = 0;
    pfds[0].events = POLLIN | POLLPRI;
    if (!write_queue_.Empty()) {
      pfds[0].events |= POLLOUT;
    }
    pfds[1].fd = pipe_[1];
    pfds[1].revents = 0;
    pfds[1].events = POLLIN;

    CHECK(keepalive_interval_sec_ >= 30) << "keepalive less than 30s";
    static const int ONE_SECOND = 1000;
    int timeout_ms = keepalive_interval_sec_ * ONE_SECOND;
    int ret = ::poll(pfds, 2, timeout_ms);
    if (ret == 0) {
      // no events
      SendHeartbeat();
    } else if (ret > 0) {
      // events come
      if (pfds[0].revents & POLLIN) {
        if (!HandleRead()) {
          break;
        }
      }
      if (pfds[0].revents & POLLOUT) {
        if (!HandleWrite()) {
          break;
        }
      }
      if (pfds[1].revents & POLLIN) {
        char c;
        while (::read(pfds[1].fd, &c, 1) == 1);
      }
    } else {
      // handle error
      error_cb_(CERROR("poll error"));
      break;
    }
  }

  is_connected_ = false;
  LOG(INFO) << "will clean and exit";
  ::shutdown(sock_fd_, SHUT_RDWR);
  if (disconnect_cb_) {
    disconnect_cb_();
  }
  LOG(INFO) << "work loop end";
}

bool SocketClient::HandleRead() {
  while (true) {
    int ret = current_read_packet_->Read(sock_fd_);
    VLOG(7) << "HandleRead ret=" << ret;
    if (ret == 0) {
      LOG(INFO) << "read eof, connection closed";
      return false;
    } else if (ret < 0) {
      if (ret == ERROR_NOMORE_DATA) {
        VLOG(5) << "no more data";
      } else {
        VLOG(5) << CERROR("system read error");
      }
      return true;
    } else {
      Json::Reader reader;
      Json::Value json;
      if(!reader.parse(current_read_packet_->Content(), json)) {
        LOG(WARNING) << "read data not json: "
                     << current_read_packet_->Content();
      }
      if (data_cb_) {
        data_cb_(current_read_packet_->Content());
      }
      current_read_packet_->Reset();
    }
  }
  return true;
}

bool SocketClient::HandleWrite() {
  VLOG(5) << "HandleWrite: size = " << write_queue_.Size();
  while (!write_queue_.Empty()) {
    PacketPtr& pkt = const_cast<PacketPtr&>(write_queue_.Front());
    int ret = pkt->Write(sock_fd_);
    if (ret == pkt->Size()) {
      PacketPtr tmp;
      bool ok = write_queue_.TryPop(tmp);
      if (!ok) {
        LOG(WARNING) << "packet maybe popped in other thread, dangerous !";
      }
      continue;
    } else if (ret != 0) {
      VLOG(5) << "write not complete: " << ret;
      return true;
    } else {
      LOG(ERROR) << "write eof, connection closed: ";
      return false;
    }
  }
  return true;
}

void SocketClient::Notify() {
  // notify through socket pipe
  static const char PIPE_DATA = '0';
  ::send(pipe_[0], &PIPE_DATA, 1, 0);
}

void SocketClient::Close() {
  is_connected_ = false;
  Notify();
}

void SocketClient::WaitForClose() {
  if (is_connected_) {
    Close();
  }
  VLOG(3) << "Join worker thread ...";
  if (worker_thread_.get()) {
    worker_thread_->Join();
  }
  VLOG(3) << "worker thread exited";
}

int SocketClient::SendHeartbeat() {
  VLOG(5) << "SendHeartbeat";
  Json::Value json;
  json["type"] = "noop";

  PacketPtr packet(new Packet());
  string data;
  SerializeJson(json, data);
  packet->SetContent(data);
  write_queue_.Push(packet);
  HandleWrite();
  return 0;
}

void SocketClient::SendJson(const Json::Value& json) {
  PacketPtr packet(new Packet());
  string data;
  SerializeJson(json, data);
  packet->SetContent(data);
  write_queue_.Push(packet);

  Notify();
}

void SocketClient::Send(string& data) {
  PacketPtr packet(new Packet());
  packet->SetContent(data);
  write_queue_.Push(packet);
  Notify();
}

}  // namespace xcomet
