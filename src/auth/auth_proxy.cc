#include "deps/base/flags.h"
#include "deps/jsoncpp/include/json/json.h"
#include "src/auth/auth_proxy.h"
#include "src/http_client.h"
#include "src/utils.h"

DEFINE_string(auth_proxy_addr, "192.168.1.187:9099", "");
const char* AUTH_PROXY_URL_PATH = "/embed/pushMsg/valid";

namespace xcomet {

AuthProxy::AuthProxy(struct event_base* evbase)
    : evbase_(evbase),
      cache_(100000) {
  LOG(INFO) << "AuthProxy()";
  ParseIpPort(FLAGS_auth_proxy_addr, proxy_ip_, proxy_port_);
  CHECK(!proxy_ip_.empty());
  CHECK(proxy_port_ > 1024 && proxy_port_ < 65536);
}

AuthProxy::~AuthProxy() {
  LOG(INFO) << "~AuthProxy()";
}

static string UrlEncode(const string& input) {
  // TODO(qingfeng) implement www-form-urlencode
  return input;
}

void AuthProxy::Authenticate(const string& user,
                             const string& password,
                             AuthCallback cb) {
  base::shared_ptr<bool> cache_value = cache_.Get(user);
  if (cache_value.get() && *cache_value) {
    cb(NO_ERROR, true);
    return;
  }
  VLOG(4) << "Authenticate input: " << user << "," << password;

  string form_data;
  form_data += "id=";
  form_data += UrlEncode(user);
  form_data += "&token=";
  form_data += UrlEncode(password);
  HttpRequestOption option = {
    proxy_ip_.c_str(),
    proxy_port_,
    "post",
    AUTH_PROXY_URL_PATH,
    form_data.c_str(),
    (int)form_data.size()
  };
  VLOG(6) << "Authenticat request option: " << option;
  HttpClient* client = new HttpClient(evbase_, option);
  client->SetRequestDoneCallback([user, cb, client, this]
                                 (Error error, StringPtr result) {
    if (error != NO_ERROR) {
      VLOG(3) << "auth proxy request failed: " << error;
      cb(error, false);
    } else {
      Json::Value json;
      try {
        if (!Json::Reader().parse(*result, json)) {
          LOG(ERROR) << "auth result format syntax error: " << *result;
          cb("auth result format syntax error", false);
        } else if (!json.isMember("success")) {
          LOG(ERROR) << "invalid auth result: " << json;
          cb("invalid auth result", false);
        } else {
          bool success = json["success"].asBool();
          cache_.Put(user, new bool(success));
          cb(NO_ERROR, success);
        }
      } catch (std::exception& e) {
        LOG(ERROR) << "parse auth result: " << *result
                   << ", cause json excpetion: " << e.what();
        cb("parse auth result cause json exception", false);
      }
    }
    delete client;
  });
  client->StartRequest();
}

}  // namespace xcomet
