#include "src/stats_manager.h"

#include <string.h>
#include "deps/base/logging.h"

namespace xcomet {

StatsManager::StatsManager(int interval) : timer_interval_(interval) {
  CHECK(interval > 0);
  ::memset(&data_, 0, sizeof(PlainData));
}

StatsManager::~StatsManager() {
}

void StatsManager::OnServerStart() {
}

void StatsManager::OnTimer() {
}

void StatsManager::OnUserMessage(const StringPtr& data) {
  ++data_.total_user_msg_number_;
  data_.total_user_msg_bytes_ += data->size();
}

void StatsManager::OnPubMessage(const StringPtr& data) {
  ++data_.total_pub_msg_number_;
  data_.total_pub_msg_bytes_ += data->size();
}

void StatsManager::GetReport(Json::Value& report) const {
  report["server_start_time"] = "1";
  Json::Value& throughput = report["throughput"];
  throughput["total_user_msg_number"] =
      (Json::Int64)data_.total_user_msg_number_;
  throughput["total_user_msg_bytes"] =
      (Json::Int64)data_.total_user_msg_bytes_;
  throughput["total_pub_msg_number"] =
      (Json::Int64)data_.total_pub_msg_number_;
  throughput["total_pub_msg_bytes"] =
      (Json::Int64)data_.total_pub_msg_bytes_;
}

}  // namespace xcomet
