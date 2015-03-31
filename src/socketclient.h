#ifndef SRC_SOCKETCLIENT_H_
#define SRC_SOCKETCLIENT_H_

#include <evhttp.h>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include "deps/base/thread.h"
#include "deps/base/shared_ptr.h"
#include "deps/base/scoped_ptr.h"
#include "deps/base/atomic.h"
#include "deps/base/concurrent_queue.h"
#include "deps/jsoncpp/include/json/json.h"
#include "src/include_std.h"

namespace xcomet {

struct ClientOption {
  std::string host;
  int port;
  std::string username;
  std::string password;
};

inline ostream& operator<<(ostream& os, const ClientOption& option) {
  os << option.host << ":" << option.port << ", "
     << option.username << "/" << option.password;
  return os;
}

const int MAX_BUFFER_SIZE = 1024;

class BufferReader {
 public:
  BufferReader();
  ~BufferReader();
  int Read(int fd, char* addr, int len);
  int ReadLine(int fd, char* addr);
  int Size() {return end_ - start_;}
  void Print();

 private:
  int Read(int fd);
  void Shrink();
  int FindCRLF();
  char buf_[MAX_BUFFER_SIZE];
  int start_;
  int end_;
};

class Packet {
 public:
  Packet();
  ~Packet();
  int Read(int fd);
  int Write(int fd);
  void SetContent(std::string& str) {
    content_.swap(str);
    content_.append("\n\r\n");
    len_ = content_.size();
  }
  int Size() const {
    return len_;
  }
  std::string& Content() {
    return content_;
  }
  void Reset() {
    len_ = 0;
    left_ = 0;
    content_.clear();
    state_ = NONE;
    ::memset(data_len_buf_, 0, sizeof(data_len_buf_));
    buf_start_ = 0;
    rstate_ = RS_HEADER;
  }

 private:
  int ReadDataLen(int fd);

  enum ReadState {
    RS_HEADER,
    RS_BODY,
  } rstate_;
  BufferReader reader_;

  int len_;
  int left_;
  // int type_;
  std::string content_;
  enum WriteState {
    NONE,
    DATA_LEN,
    DATA_BODY,
  };
  WriteState state_;
  static const int MAX_DATA_LEN = 20;
  char data_len_buf_[MAX_DATA_LEN];
  int buf_start_;
};

typedef boost::function<void ()> ConnectCallback;
typedef boost::function<void ()> DisconnectCallback;
typedef boost::function<void (std::string&)> DataCallback;
typedef boost::function<void (const std::string&)> ErrorCallback;

class SocketClient {
 public:
  SocketClient(const ClientOption& option);
  virtual ~SocketClient();

  void SetConnectCallback(const ConnectCallback& cb) {
    connect_cb_ = cb;
  }
  void SetDisconnectCallback(const DisconnectCallback& cb) {
    disconnect_cb_ = cb;
  }
  void SetDataCallback(const DataCallback& cb) {
    data_cb_ = cb;
  }
  void SetErrorCallback(const ErrorCallback& cb) {
    error_cb_ = cb;
  }

  void SetHost(const std::string& host) {
    option_.host = host;
  }

  void SetPort(int port) {
    option_.port = port;
  }

  void SetUserName(const std::string& username) {
    option_.username = username;
  }

  void SetPassword(const std::string& password) {
    option_.password = password;
  }

  const ClientOption& GetOption() const {
    return option_;
  }

  void SetKeepaliveInterval(int interval_sec) {
    keepalive_interval_sec_ = interval_sec;
  }

  int Connect();
  void Close();
  void Send(string& data);  // maybe swap
  void SendJson(const Json::Value& value);
  void WaitForClose();
  bool IsConnected() {
    return is_connected_;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SocketClient);
  void Loop();
  bool HandleRead();
  bool HandleWrite();
  void Notify();
  int SendHeartbeat();

  int sock_fd_;
  scoped_ptr<base::ThreadRunner> worker_thread_;
  ClientOption option_;
  bool is_connected_;

  ConnectCallback connect_cb_;
  DisconnectCallback disconnect_cb_;
  DataCallback data_cb_;
  ErrorCallback error_cb_;

  typedef base::shared_ptr<Packet> PacketPtr;
  base::ConcurrentQueue<PacketPtr> write_queue_;
  PacketPtr current_read_packet_;

  int pipe_[2]; 
  int keepalive_interval_sec_;
};
}  // namespace xcomet

#endif  // SRC_SOCKETCLIENT_H_
