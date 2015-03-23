#ifndef SRC_STATS_MANAGER_H_
#define SRC_STATS_MANAGER_H_

#include "deps/jsoncpp/include/json/json.h"
#include "deps/base/basictypes.h"
#include "deps/base/time.h"
#include "src/include_std.h"
#include "src/typedef.h"

namespace xcomet {

class StatsManager {
 public:
  StatsManager(int interval);
  ~StatsManager();
  void OnTimer(int64 user_number);
  void OnServerStart();
  void OnReceive(const string& data);
  void OnSend(const string& data);

  void GetReport(Json::Value& report) const;

 private:
  const int timer_interval_;
  base::Time server_start_time_;
  string server_start_datetime_;
  struct PlainData {
    int64 total_recv_number_;
    int64 last_recv_number_;
    int64 total_recv_bytes_;
    int64 last_recv_bytes_;
    int64 total_send_number_;
    int64 last_send_number_;
    int64 total_send_bytes_;
    int64 last_send_bytes_;
    int64 max_recv_number_per_second_;
    int64 max_recv_bytes_per_second_;
    int64 max_send_number_per_second_;
    int64 max_send_bytes_per_second_;
    int64 user_number_;
    int64 max_user_number_;
    int64 max_user_growth_per_second_;
    int64 max_user_reduce_per_second_;
  } d_;
};
}  // namespace xcomet
#endif  // SRC_STATS_MANAGER_H_
