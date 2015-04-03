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
                       const std::string& res = "") {
  std::string error_header;
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
  LOG(ERROR) << error_header << ": " << res;
  evhttp_add_header(req->output_headers,
                    "Content-Type",
                    "text/json; charset=utf-8");
  struct evbuffer * output_buffer = evhttp_request_get_output_buffer(req);
  const char* eptr = res.empty() ? error_header.c_str() : res.c_str();
  evbuffer_add_printf(output_buffer, "{\"error\":\"%s\"}\n", eptr);
  evhttp_send_reply(req, code, error_header.c_str(), output_buffer);
}

}  // namespace xcomet

#endif  // SRC_EVHELPER_H_
