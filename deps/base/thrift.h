// for conveniently use the thrift

#ifndef BASE_THRIFT_H_
#define BASE_THRIFT_H_

#include <string>
#include <utility>

#include <boost/shared_ptr.hpp>

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/thread.h"
#include "base/string_util.h"
#include <protocol/TBinaryProtocol.h>
#include <protocol/TDebugProtocol.h>
#include <protocol/TJSONProtocol.h>
#include <transport/TBufferTransports.h>
#include <transport/TSocket.h>
// #include <server/TNonblockingServer.h>
#include <concurrency/PosixThreadFactory.h>
#include <concurrency/ThreadManager.h>

using namespace ::apache::thrift;  // NOLINT
using namespace ::apache::thrift::protocol;  // NOLINT
using namespace ::apache::thrift::transport;  // NOLINT
// using namespace ::apache::thrift::server;  // NOLINT

namespace apache {
namespace thrift {
namespace concurrency {
class PosixThreadFactory;
class ThreadManager;
}
}
}

namespace base {

#if 0
// create the server in another thread and run.
template <typename HandlerT, typename ProcessorT>
class ThriftNonBlockingServerThread : public Thread {
 public:
  ThriftNonBlockingServerThread(bool joinable, int port, HandlerT *handler)
      : Thread(joinable), port_(port), handler_(handler) {
  }

  uint16 ServerPort() {
    if (server_.get()) {
      return server_->getServerPort();
    } else {
      LOG(ERROR) << "failed to get local port, server is null";
      return 0;
    }
  }

 private:
  virtual void Run() {
    if (handler_.get() == NULL)
      handler_.reset(new HandlerT());
    processor_.reset(new ProcessorT(handler_));
    server_.reset(new TNonblockingServer(processor_, port_));
    LOG(INFO) << "start listening on port:" << port_;
    server_->serve();
  }

  int port_;
  scoped_ptr<TNonblockingServer> server_;
  boost::shared_ptr<HandlerT> handler_;
  boost::shared_ptr<ProcessorT> processor_;
  DISALLOW_COPY_AND_ASSIGN(ThriftNonBlockingServerThread);
};

template <typename HandlerT, typename ProcessorT>
class ThriftNonBlockingServerMutiThread : public Thread {
 public:
  ThriftNonBlockingServerMutiThread(bool joinable, int port,
                                    HandlerT *handler, int thread_num)
      : Thread(joinable), port_(port),
        thread_num_(thread_num), handler_(handler) {
  }

  uint16 ServerPort() {
    if (server.get()) {
      return server->getServerPort();
    } else {
      LOG(ERROR) << "failed to get local port, server is null";
      return 0;
    }
  }

 private:
  virtual void Run() {
    if (handler_.get() == NULL) {
      LOG(ERROR) << "handler is null";
      return;
    }
    processor_.reset(new ProcessorT(handler_));
    proto_factory_.reset(new TBinaryProtocolFactory());
    thread_manager_ = apache::thrift::concurrency::ThreadManager::
                      newSimpleThreadManager(thread_num_);
    boost::shared_ptr< apache::thrift::concurrency::
                      PosixThreadFactory> thread_factory;
    thread_factory.reset(new apache::thrift::concurrency::PosixThreadFactory());
    thread_manager_->threadFactory(thread_factory);
    thread_manager_->start();

    server.reset(new TNonblockingServer(processor_, proto_factory_,
                                        port_, thread_manager_));
    server->serve();
  }

  int port_;
  int thread_num_;
  scoped_ptr<TNonblockingServer> server;
  boost::shared_ptr<HandlerT> handler_;
  boost::shared_ptr<ProcessorT> processor_;
  boost::shared_ptr<TProtocolFactory> proto_factory_;
  boost::shared_ptr<apache::thrift::concurrency::ThreadManager> thread_manager_;
  DISALLOW_COPY_AND_ASSIGN(ThriftNonBlockingServerMutiThread);
};
#endif

// the following functions is just for simply using
// The type T must be a thrift object.
template <typename T>
const std::string FromThriftToString(const T *object) {
  boost::shared_ptr<TMemoryBuffer> membuffer(new TMemoryBuffer());
  scoped_ptr<TProtocol> protocol(new TBinaryProtocol(membuffer));
  object->write(protocol.get());
  uint8* buffer = NULL;
  uint32 buffer_size = 0;
  membuffer->getBuffer(&buffer, &buffer_size);
  return std::string(reinterpret_cast<const char*>(buffer), buffer_size);
}

// This method is unsafe and could be used only when object is clean.
template <typename T>
inline bool FromStringToThriftFast(const std::string &buffer, T *object) {
  boost::shared_ptr<TMemoryBuffer> membuffer(new TMemoryBuffer(
      const_cast<uint8*>(reinterpret_cast<const uint8*>(buffer.c_str())),
      buffer.size()));
  scoped_ptr<TProtocol> binaryprotocol(new TBinaryProtocol(membuffer));
  try {
    object->read(binaryprotocol.get());
  } catch(const TException &ex) {
    LOG(ERROR) << ex.what();
    return false;
  }
  return true;
}

template <typename T>
bool FromStringToThrift(const std::string &buffer, T *object) {
  T tmp_object;
  if (FromStringToThriftFast(buffer, &tmp_object)) {
    *object = tmp_object;
    return true;
  }
  return false;
}

template <typename T>
const std::string FromThriftToDebugString(const T *object) {
  boost::shared_ptr<TMemoryBuffer> membuffer(new TMemoryBuffer());
  scoped_ptr<TProtocol> protocol(new TDebugProtocol(membuffer));
  object->write(protocol.get());
  uint8* buffer = NULL;
  uint32 buffer_size = 0;
  membuffer->getBuffer(&buffer, &buffer_size);
  return std::string(reinterpret_cast<const char*>(buffer), buffer_size);
}

template <typename T>
const std::string FromThriftToUtf8DebugString(const T *object) {
  std::string debug_string = FromThriftToDebugString(object);
  debug_string.append(" ");
  std::string out;
  for (int i = 0; i < debug_string.length();) {
    if (debug_string[i] == '\\') {
      switch (debug_string[i + 1]) {
        case 'x': {
          std::string hex_string;
          hex_string.insert(hex_string.end(), debug_string[i + 2]);
          hex_string.insert(hex_string.end(), debug_string[i + 3]);
          int hex_value = 0;
          HexStringToInt(hex_string, &hex_value);
          out.insert(out.end(), static_cast<unsigned char>(hex_value));
          i += 4;
          break;
        }
        case 'a': out.insert(out.end(), '\a'); i += 2; break;
        case 'b': out.insert(out.end(), '\b'); i += 2; break;
        case 'f': out.insert(out.end(), '\f'); i += 2; break;
        case 'n': out.insert(out.end(), '\n'); i += 2; break;
        case 'r': out.insert(out.end(), '\r'); i += 2; break;
        case 't': out.insert(out.end(), '\t'); i += 2; break;
        case 'v': out.insert(out.end(), '\v'); i += 2; break;
        case '\\': out.insert(out.end(), '\\'); i += 2; break;
        case '"': out.insert(out.end(), '"'); i += 2; break;
      }
    } else {
      out.insert(out.end(), debug_string[i]);
      i++;
    }
  }
  return out;
}

template <typename T>
void FromDebugStringToThrift(const std::string &buffer, T *object) {
  boost::shared_ptr<TMemoryBuffer> membuffer(new TMemoryBuffer(
      const_cast<uint8*>(reinterpret_cast<const uint8*>(buffer.c_str())),
      buffer.size()));
  scoped_ptr<TProtocol> binaryprotocol(new TDebugProtocol(membuffer));
  T tmp_object;
  tmp_object.read(binaryprotocol.get());
  *object = tmp_object;
}

template <typename T>
const std::string FromThriftToJson(const T *object) {
  boost::shared_ptr<TMemoryBuffer> membuffer(new TMemoryBuffer());
  scoped_ptr<TProtocol> protocol(new TJSONProtocol(membuffer));
  object->write(protocol.get());
  uint8* buffer = NULL;
  uint32 buffer_size = 0;
  membuffer->getBuffer(&buffer, &buffer_size);
  return std::string(reinterpret_cast<const char*>(buffer), buffer_size);
}

template <typename T>
void FromJsonToThrift(const std::string &buffer, T *object) {
  boost::shared_ptr<TMemoryBuffer> membuffer(new TMemoryBuffer(
      const_cast<uint8*>(reinterpret_cast<const uint8*>(buffer.c_str())),
      buffer.size()));
  scoped_ptr<TProtocol> binaryprotocol(new TJSONProtocol(membuffer));
  T tmp_object;
  tmp_object.read(binaryprotocol.get());
  *object = tmp_object;
}
// ----------------------------------------------------------------------

// The following two class is for someone
// who want to reuse the membuffer and protocal.
// Be careful These classes are not thread-safe
class ThriftObjReader {
 public:
  explicit ThriftObjReader()
    : membuffer_(new TMemoryBuffer),
      protocol_(new TBinaryProtocol(membuffer_)) {
  }
  ~ThriftObjReader() {}

  // This method is unsafe and could be used only when object is clean.
  template <typename T>
  bool FromStringToThriftFast(const std::string &buffer, T* object) {
    // DCHECK(*object == T()) << "This method must be called with clean object";
    membuffer_->resetBuffer(
        const_cast<uint8*>(reinterpret_cast<const uint8*>(buffer.c_str())),
        buffer.size());
    try {
      object->read(protocol_.get());
    } catch(const TException &ex) {
      LOG(ERROR) << ex.what();
      return false;
    }
    return true;
  }

  template <typename T>
  bool FromStringToThrift(const std::string &buffer, T* object) {
    T tmp_object;
    if (FromStringToThriftFast(buffer, &tmp_object)) {
      *object = tmp_object;
      return true;
    }
    return false;
  }

 private:
  boost::shared_ptr<TMemoryBuffer> membuffer_;
  scoped_ptr<TProtocol> protocol_;
  DISALLOW_COPY_AND_ASSIGN(ThriftObjReader);
};

class ThriftObjWriter {
 public:
  ThriftObjWriter()
    : membuffer_(new TMemoryBuffer),
      protocol_(new TBinaryProtocol(membuffer_)) {
  }
  ~ThriftObjWriter() {}

  template <typename T>
  bool FromThriftToString(
      const T& object, uint8_t** buf, uint32_t* buf_size) {
    membuffer_->resetBuffer();
    try {
      object.write(protocol_.get());
    } catch(const TException &ex) {
      LOG(ERROR) << ex.what();
      return false;
    }
    *buf = NULL;
    *buf_size = 0;
    membuffer_->getBuffer(buf, buf_size);
    return true;
  }

  template <typename T>
  bool FromThriftToString(const T& object, std::string* buffer) {
    uint8_t* buf = NULL;
    uint32_t buf_size = 0;
    if (!FromThriftToString(object, &buf, &buf_size)) {
      return false;
    }
    buffer->assign(reinterpret_cast<const char*>(buf), buf_size);
    return true;
  }

 private:
  boost::shared_ptr<TMemoryBuffer> membuffer_;
  scoped_ptr<TProtocol> protocol_;
  DISALLOW_COPY_AND_ASSIGN(ThriftObjWriter);
};
// ----------------------------------------------------------------------

// A thrift service client wrapper which will automatically handle the
// re-connection logics
template <typename Service>
class ThriftClient {
 public:
  ThriftClient(const std::string &host, int port)
      : host_(host), port_(port), connected_(false) {
        connection_timeout_ = kConnectionTimeout;
        receive_timeout_ = kReceiveTimeout;
        send_timeout_ = kSendTimeout;
        transport_class_ = "TFramedTransport";
      }

  void SetConnectionTimeout(uint32 timeout) {
    connection_timeout_ = timeout;
  }

  void SetReceiveTimeout(uint32 timeout) {
    receive_timeout_ = timeout;
  }

  void SetSendTimeout(uint32 timeout) {
    send_timeout_ = timeout;
  }

  void SetFrontendPort(int32 frontend_port) {
    frontend_port_ = frontend_port;
  }

  virtual ~ThriftClient() {}

  // Get the real service client, do call this method everytime you need the
  // client, so that this class could handle re-connect logics for you.
  // Making the function virtual is for Mock
  virtual Service *GetService() {
    // TODO(dahaili): avoid re-connect too many times in a short time span.
    ConnectIfNecessary();
    return service_.get();
  }

  // Flag the conntected to false if call service failed by any reason.
  void CallServiceFailed() {
    connected_ = false;
  }

  // get transport to close
  boost::shared_ptr<TTransport> GetTransport() {
    return transport_;
  }

  void SetTransportClass(const std::string &transport_class) {
    if (transport_class_ != transport_class) {
      transport_class_ = transport_class;
      ConnectIfNecessary();
    }
  }

  // get host port
  std::pair<std::string, int> GetReplica() {
    return make_pair(host_, port_);
  }

  // get host, port, frontend port
  int GetFrontendPort() {
    return frontend_port_;
  }

 private:
  static const uint32 kFrameSize = 128 * 1024;
  // try connect the crawler if the connection is broken or not created.
  void ConnectIfNecessary() {
    if (connected_) return;

    protocol_.reset(static_cast<TBinaryProtocol*>(NULL));
    service_.reset(static_cast<Service*>(NULL));
    socket_.reset(new TSocket(host_, port_));
    socket_->setConnTimeout(connection_timeout_);
    socket_->setRecvTimeout(receive_timeout_);
    socket_->setSendTimeout(send_timeout_);

    if (transport_class_ == "TFramedTransport") {
      transport_.reset(new TFramedTransport(socket_, kFrameSize));
    } else if (transport_class_ == "TBufferedTransport") {
      transport_.reset(new TBufferedTransport(socket_, kFrameSize));
    } else {
      LOG(WARNING) << "Unsupported transport (" << transport_class_
          << ") use TFramedTransport as default";
      transport_.reset(new TFramedTransport(socket_, kFrameSize));
    }

    try {
      transport_->open();
    } catch(const TTransportException& tx) {
      LOG(ERROR) << "failed to connect to " << host_ << ":" << port_
        << " for " << tx.what();
      return;
    } catch(const TException& tx) {
      LOG(ERROR) << "failed to connect to " << host_ << ":" << port_
        << " for " << tx.what();
      return;
    } catch(...) {
      LOG(ERROR) << "failed to connect to " << host_ << ":" << port_;
      return;
    }
    protocol_.reset(new TBinaryProtocol(transport_));
    service_.reset(new Service(protocol_));
    connected_ = true;
  }

 private:
  std::string host_;
  int port_;
  // mini frontend port, for debug
  int frontend_port_;
  bool connected_;
  std::string transport_class_;

  static const uint32 kConnectionTimeout = 500;
  static const uint32 kReceiveTimeout = 10000;
  static const uint32 kSendTimeout = 10000;

  uint32 connection_timeout_;
  uint32 receive_timeout_;
  uint32 send_timeout_;

  boost::shared_ptr<TSocket> socket_;
  boost::shared_ptr<TTransport> transport_;
  boost::shared_ptr<TProtocol> protocol_;
  boost::shared_ptr<Service> service_;

  DISALLOW_COPY_AND_ASSIGN(ThriftClient);
};

}  // namespace base

#endif  // BASE_THRIFT_H_
