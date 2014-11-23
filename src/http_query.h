#ifndef SRC_HTTP_QUERY_H_
#define SRC_HTTP_QUERY_H_

#include <evhttp.h>

namespace xcomet {
class HttpQuery {
 public:
	HttpQuery(const struct evhttp_request *req) {
		evhttp_parse_query(evhttp_request_get_uri(req), &params);
	}
	~HttpQuery() {
		evhttp_clear_headers(&params);
	}
	int GetInt(const char* key, int default_value) {
		const char* val = evhttp_find_header(&params, key);
		return val ?atoi(val) :default_value;
	}
	const char* GetStr(const char* name, const char* default_value) const {
		const char* val = evhttp_find_header(&params, name);
		return val ?val :default_value;
	}
 private:
	struct evkeyvalq params;
};
}  // namespace xcomet

#endif  // SRC_HTTP_QUERY_H_
