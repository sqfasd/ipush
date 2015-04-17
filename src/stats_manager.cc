#include "src/stats_manager.h"

#include <string.h>
#include "deps/base/logging.h"

namespace xcomet {

StatsManager::StatsManager(int interval) : timer_interval_(interval) {
  CHECK(interval > 0);
  ::memset(&d_, 0, sizeof(PlainData));
  CHECK(Message::T_COUNT > 0);
  recv_msg_type_count_.resize(Message::T_COUNT);
  for (int i = 0; i < recv_msg_type_count_.size(); ++i) {
    recv_msg_type_count_[i] = 0;
  }
}

StatsManager::~StatsManager() {
}

void StatsManager::OnServerStart() {
  server_start_time_ = base::Time::Now();
  server_start_time_.ToLocalString(&server_start_datetime_);
}

void StatsManager::OnTimer(int64 user_number) {
  int recv_num_delta =
      (d_.total_recv_number - d_.last_recv_number) / timer_interval_;
  if (recv_num_delta > d_.max_recv_number_per_second) {
    d_.max_recv_number_per_second = recv_num_delta;
  }
  int recv_bytes_delta =
      (d_.total_recv_bytes - d_.last_recv_bytes) / timer_interval_;
  if (recv_bytes_delta > d_.max_recv_bytes_per_second) {
    d_.max_recv_bytes_per_second = recv_bytes_delta;
  }
  int send_num_delta =
      (d_.total_send_number - d_.last_send_number) / timer_interval_;
  if (send_num_delta > d_.max_send_number_per_second) {
    d_.max_send_number_per_second = send_num_delta;
  }
  int send_bytes_delta =
      (d_.total_send_bytes - d_.last_send_bytes) / timer_interval_;
  if (send_bytes_delta > d_.max_send_bytes_per_second) {
    d_.max_send_bytes_per_second = send_bytes_delta;
  }
  if (user_number > d_.max_user_number) {
    d_.max_user_number = user_number;
  }
  int user_growth = user_number - d_.user_number;
  if (user_growth > d_.max_user_growth_per_second) {
    d_.max_user_growth_per_second = user_growth;
  }
  if (user_growth < d_.max_user_reduce_per_second) {
    d_.max_user_reduce_per_second = user_growth;
  }

  d_.last_recv_number = d_.total_recv_number;
  d_.last_recv_bytes = d_.total_recv_bytes;
  d_.last_send_number = d_.total_send_number;
  d_.last_send_bytes = d_.total_send_bytes;
  d_.user_number = user_number;
}

void StatsManager::GetReport(Json::Value& report) const {
  report["server_start_timestamp"] =
      (Json::Int64)server_start_time_.ToInternalValue();
  report["server_start_datetime"] = server_start_datetime_;
  report["error_number"] = (Json::Int64)d_.error_number;
  report["bad_request_number"] = (Json::Int64)d_.bad_request_number;
  report["redirect_number"] = (Json::Int64)d_.redirect_number;
  report["auth_fail_number"] = (Json::Int64)d_.auth_fail_number;

  int64 seconds = (base::Time::Now() - server_start_time_).InSeconds();
  CHECK(seconds >= 0);
  if (seconds == 0) {
    seconds = 1;
  }
  Json::Value& tp = report["throughput"];
  tp["total_recv_number"] = (Json::Int64)d_.total_recv_number;
  tp["total_recv_bytes"] = (Json::Int64)d_.total_recv_bytes;
  tp["total_send_number"] = (Json::Int64)d_.total_send_number;
  tp["total_send_bytes"] = (Json::Int64)d_.total_send_bytes;
  tp["avg_recv_number_per_second"] =
      (Json::Int64)d_.total_recv_number / seconds;
  tp["avg_recv_bytes_per_second"] =
      (Json::Int64)d_.total_recv_bytes / seconds;
  tp["avg_send_number_per_second"] =
      (Json::Int64)d_.total_send_number / seconds;
  tp["avg_send_bytes_per_second"] =
      (Json::Int64)d_.total_send_bytes / seconds;
  tp["max_recv_number_per_second"] =
      (Json::Int64)d_.max_recv_number_per_second;
  tp["max_recv_bytes_per_second"] =
      (Json::Int64)d_.max_recv_bytes_per_second;
  tp["max_send_number_per_second"] =
      (Json::Int64)d_.max_send_number_per_second;
  tp["max_send_bytes_per_second"] =
      (Json::Int64)d_.max_send_bytes_per_second;

  Json::Value& user = report["user"];
  user["user_number"] = (Json::Int64)d_.user_number;
  user["max_user_number"] = (Json::Int64)d_.max_user_number;
  user["max_user_growth_per_second"] =
      (Json::Int64)d_.max_user_growth_per_second;
  user["max_user_reduce_per_second"] =
      (Json::Int64)d_.max_user_reduce_per_second;
  user["connect"] = (Json::Int64)d_.user_connect;
  user["reconnect"] = (Json::Int64)d_.user_reconnect;
  user["disconnect"] = (Json::Int64)d_.user_disconnect;

  Json::Value& request = report["request"];
  for (auto& kv : req_count_) {
    request[kv.first] = (Json::Int64)kv.second;
  }

  Json::Value& recv_type = report["recv_type"];
  for (int i = 0; i < recv_msg_type_count_.size(); ++i) {
    recv_type[Message::EnumTypeToString((Message::MType)i)] =
        (Json::Int64)recv_msg_type_count_[i];
  }
}

}  // namespace xcomet
