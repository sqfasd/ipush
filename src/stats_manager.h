#ifndef SRC_STATS_MANAGER_H_
#define SRC_STATS_MANAGER_H_

#include "deps/jsoncpp/include/json/json.h"
#include "deps/base/basictypes.h"
#include "deps/base/time.h"
#include "src/include_std.h"
#include "src/typedef.h"
#include "src/message.h"

namespace xcomet {

class StatsManager {
 public:
  StatsManager(int interval);
  ~StatsManager();
  void OnTimer(int64 user_number);
  void OnServerStart();
  void OnReceive(const string& data, const Message& msg) {
    ++d_.total_recv_number;
    d_.total_recv_bytes += data.size();
    ++recv_msg_type_count_[msg.Type()];
  }
  void OnSend(const string& data) {
    ++d_.total_send_number;
    d_.total_send_bytes += data.size();
  }
  void OnRequest(const char* request) {
    ++req_count_[request];
  }
  void OnError() {
    ++d_.error_number;
  }
  void OnAuthFailed() {
    ++d_.auth_fail_number;
  }

  void OnBadRequest() {
    ++d_.bad_request_number;
  }
  void OnRedirect() {
    ++d_.redirect_number;
  }
  void OnUserConnect() {
    ++d_.user_connect;
  }
  void OnUserReconnect() {
    ++d_.user_reconnect;
  }
  void OnUserDisconnect() {
    ++d_.user_disconnect;
  }

  void GetReport(Json::Value& report) const;

 private:
  const int timer_interval_;
  base::Time server_start_time_;
  string server_start_datetime_;
  struct PlainData {
    int64 total_recv_number;
    int64 last_recv_number;
    int64 total_recv_bytes;
    int64 last_recv_bytes;
    int64 total_send_number;
    int64 last_send_number;
    int64 total_send_bytes;
    int64 last_send_bytes;
    int64 max_recv_number_per_second;
    int64 max_recv_bytes_per_second;
    int64 max_send_number_per_second;
    int64 max_send_bytes_per_second;
    int64 user_number;
    int64 max_user_number;
    int64 max_user_growth_per_second;
    int64 max_user_reduce_per_second;
    int64 error_number;
    int64 bad_request_number;
    int64 redirect_number;
    int64 auth_fail_number;
    int64 user_connect;
    int64 user_reconnect;
    int64 user_disconnect;
  } d_;
  vector<int64> recv_msg_type_count_;
  unordered_map<string, int64> req_count_;
};
}  // namespace xcomet
#endif  // SRC_STATS_MANAGER_H_
