#ifndef SRC_HTTP_CLIENT_H_
#define SRC_HTTP_CLIENT_H_

#include <assert.h>
#include <string>
#include <functional>
#include <memory>
#include "include_std.h"
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/http.h>
#include <evhttp.h>

namespace xcomet {

struct HttpClientOption {
  int id;
  std::string host;
  int port;
  enum evhttp_cmd_type method;
  std::string path;
  std::string data;
  HttpClientOption(): 
    id(-1), host(), port(-1), method(), path(), data("") {
  }
};

inline ostream& operator << (ostream& os, enum evhttp_cmd_type cmd_type) {
  switch(cmd_type) {
    case EVHTTP_REQ_GET:
        return os << "GET";
    case EVHTTP_REQ_POST:
        return os << "POST";
    default:
        return os << "OTHERS";
  }
}

inline ostream& operator << (ostream& os, const HttpClientOption& option) {
  os << "id:" << option.id << ","
     << "host:" << option.host << ","
     << "port:" << option.port << ","
     << "method:" << option.method << ","
     << "path:" << option.path << ","
     << "data:" << option.data;
  return os;
}

class HttpClient;

typedef void (*ChunkCallback)(const HttpClient* client, const string&, void *ctx);
typedef void (*RequestDoneCallback)(const HttpClient* client, const string&, void *ctx);
typedef void (*CloseCallback)(const HttpClient* client, void *ctx);

typedef void (*EventCallback)(int sock, short which, void *ctx);

class HttpClient {
 public:
  HttpClient(struct event_base* evbase, const HttpClientOption& option, void *cb_arg);
  ~HttpClient();

  void Init();
  void Free();

  void SetRequestDoneCallback(RequestDoneCallback cb) {
    request_done_cb_ = cb;
  }
  void SetChunkCallback(ChunkCallback cb) {
    chunk_cb_ = cb;
  }
  void SetCloseCallBack(CloseCallback cb) {
    close_cb_ = cb;
  }
  void StartRequest();
  void Send(const string& data);
  void SendChunk(const string& data);

  const HttpClientOption& GetOption() const {
    return option_;
  }

  void DelayRetry(EventCallback cb);
  static void OnRetry(int sock, short which, void * ctx);

 private:
  static void OnRequestDone(struct evhttp_request* req, void* ctx);
  static void OnChunk(struct evhttp_request* req, void* ctx);
  static void OnClose(struct evhttp_connection* conn, void *ctx);

  static bool IsResponseOK(evhttp_request* req);

  struct event_base* evbase_;
  struct evhttp_connection* evconn_;
  struct evhttp_request* evreq_;

  HttpClientOption option_;

  RequestDoneCallback request_done_cb_;
  ChunkCallback chunk_cb_;
  CloseCallback close_cb_;
  void *cb_arg_;
};

} // namespace xcomet

#endif  // HTTP_CLIENT_H_
