#ifndef SRC_STATS_MANAGER_H_
#define SRC_STATS_MANAGER_H_

#include "deps/base/basictypes.h"
#include "deps/jsoncpp/include/json/json.h"
#include "src/include_std.h"
#include "src/typedef.h"

namespace xcomet {

class StatsManager {
 public:
  StatsManager(int interval);
  ~StatsManager();
  void OnTimer();
  void OnServerStart();
  void OnUserMessage(const StringPtr& data);
  void OnPubMessage(const StringPtr& data);

  void GetReport(Json::Value& report) const;

 private:
  const int timer_interval_;
  struct PlainData {
    int64 server_start_timestamp_;
    int64 total_user_msg_number_;
    int64 total_user_msg_bytes_;
    int64 total_pub_msg_number_;
    int64 total_pub_msg_bytes_;
  } data_;
};
}  // namespace xcomet
#endif  // SRC_STATS_MANAGER_H_
