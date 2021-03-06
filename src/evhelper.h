#ifndef SRC_EVHELPER_H_
#define SRC_EVHELPER_H_

#include <evhttp.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <string>
#include "deps/base/logging.h"

namespace xcomet {

inline void ReplyOK(struct evhttp_request* req, const std::string& res = "") {
  evhttp_add_header(req->output_headers,
                    "Content-Type",
                    "text/json; charset=utf-8");
  struct evbuffer * output_buffer = evhttp_request_get_output_buffer(req);
  if (res.empty()) {
    evbuffer_add_printf(output_buffer, "{\"result\":\"ok\"}\n");
  } else {
    evbuffer_add(output_buffer, res.c_str(), res.size());
  }
  evhttp_send_reply(req, HTTP_OK, "OK", output_buffer);
}

inline void ReplyError(struct evhttp_request* req,
                       int code,
                       const char* reason = NULL) {
  const char* error_header = NULL;
  switch (code) {
    case HTTP_BADREQUEST:  // 400
      error_header = "Bad Request";
      break;
    case HTTP_NOTFOUND:  // 404
      error_header = "Not Found";
      break;
    case HTTP_BADMETHOD:  // 405
      error_header = "Method Not Allowed";
      break;
    case HTTP_INTERNAL:  // 500
      error_header = "Internal Error";
      break;
    default:
      error_header = "Unknwo Error";
      break;
  }
  LOG(ERROR) << error_header << ": " << (reason ? reason : "--")
             << ", url: " << evhttp_request_get_uri(req);
  evhttp_add_header(req->output_headers,
                    "Content-Type",
                    "text/json; charset=utf-8");
  struct evbuffer * output_buffer = evhttp_request_get_output_buffer(req);
  if (reason != NULL) {
    if (reason[0] == '{') {
      evbuffer_add_printf(output_buffer, "%s", reason);
    } else {
      evbuffer_add_printf(output_buffer, "{\"error\":\"%s\"}\n", reason);
    }
  } else {
    evbuffer_add_printf(output_buffer, "{\"error\":\"%s\"}\n", error_header);
  }
  evhttp_send_reply(req, code, error_header, output_buffer);
}

inline void ReplyRedirect(struct evhttp_request* req,
                          const string& addr) {
  string location;
  location.append("http://");
  location.append(addr);
  location.append(evhttp_request_get_uri(req));
  evhttp_add_header(req->output_headers, "Location", location.c_str());
  evhttp_add_header(req->output_headers, "Content-Length", "0");
  evhttp_send_reply(req, 303, NULL, NULL);
  evhttp_send_reply_end(req);
}

inline bool IsWebSocketRequest(struct evhttp_request* req) {
  struct evkeyvalq* headers = evhttp_request_get_input_headers(req);
  const char* ws_field = evhttp_find_header(headers, "Upgrade");
  return ws_field != NULL && !strcmp(ws_field, "websocket");
}

}  // namespace xcomet

#endif  // SRC_EVHELPER_H_
