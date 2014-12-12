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
  std::string host;
  int port;
  enum evhttp_cmd_type method;
  std::string path;
  std::string data;
};

class HttpClient;
//typedef std::function<void (const HttpClient*, const std::string&)> RequestDoneCallback;
//typedef std::function<void (const HttpClient*, const std::string&)> ChunkCallback;

typedef void (*ChunkCallback)(const HttpClient*, const string&);
typedef void (*RequestDoneCallback)(const HttpClient*, const string&);
typedef void (*CloseCallback)(const HttpClient*);

class HttpClient {
 public:
  HttpClient(struct event_base* evbase, const HttpClientOption& option);
  ~HttpClient();
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

 private:
  static void OnRequestDone(struct evhttp_request* req, void* ctx);
  static void OnChunk(struct evhttp_request* req, void* ctx);
  static void OnClose(struct evhttp_connection* conn, void *ctx);

  struct event_base* evbase_;
  struct evhttp_connection* evconn_;

  RequestDoneCallback request_done_cb_;
  ChunkCallback chunk_cb_;
  CloseCallback close_cb_;

  HttpClientOption option_;
};

} // namespace xcomet

#endif  // HTTP_CLIENT_H_
