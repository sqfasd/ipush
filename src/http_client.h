#ifndef SRC_HTTP_CLIENT_H_
#define SRC_HTTP_CLIENT_H_

#include <assert.h>
#include <event.h>
#include <evhttp.h>
#include <string>
#include <functional>
#include <memory>

#include "src/typedef.h"

namespace xcomet {

struct HttpRequestOption {
  const char* host;
  int port;
  const char* method;
  const char* path;
  const char* data;
  int data_len;
};

class HttpClient;
typedef std::function<void (Error error, StringPtr result)> RequestDoneCallback;
typedef std::function<void (const HttpClient*, StringPtr data)> ChunkCallback;

class HttpClient {
 public:
  HttpClient(struct event_base* evbase, const HttpRequestOption& option);
  ~HttpClient();
  void SetRequestDoneCallback(const RequestDoneCallback& cb) {
    request_done_cb_ = cb;
  }
  void SetChunkCallback(ChunkCallback cb) {
    chunk_cb_ = cb;
  }
  void StartRequest();

 private:
  static void OnRequestDone(struct evhttp_request* req, void* ctx);
  static void OnChunk(struct evhttp_request* req, void* ctx);

  struct event_base* evbase_;
  const HttpRequestOption option_;
  struct evhttp_connection* evconn_;
  struct evhttp_request* evreq_;
  RequestDoneCallback request_done_cb_;
  ChunkCallback chunk_cb_;
};

} // namespace xcomet
#endif  // SRC_HTTP_CLIENT_H_

