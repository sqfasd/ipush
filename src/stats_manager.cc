#include "src/stats_manager.h"

#include <string.h>
#include "deps/base/logging.h"

namespace xcomet {

StatsManager::StatsManager(int interval) : timer_interval_(interval) {
  CHECK(interval > 0);
  ::memset(&d_, 0, sizeof(PlainData));
}

StatsManager::~StatsManager() {
}

void StatsManager::OnServerStart() {
  server_start_time_ = base::Time::Now();
  server_start_time_.ToLocalString(&server_start_datetime_);
}

void StatsManager::OnTimer(int64 user_number) {
  int recv_num_delta =
      (d_.total_recv_number_ - d_.last_recv_number_) / timer_interval_;
  if (recv_num_delta > d_.max_recv_number_per_second_) {
    d_.max_recv_number_per_second_ = recv_num_delta;
  }
  int recv_bytes_delta =
      (d_.total_recv_bytes_ - d_.last_recv_bytes_) / timer_interval_;
  if (recv_bytes_delta > d_.max_recv_bytes_per_second_) {
    d_.max_recv_bytes_per_second_ = recv_bytes_delta;
  }
  int send_num_delta =
      (d_.total_send_number_ - d_.last_send_number_) / timer_interval_;
  if (send_num_delta > d_.max_send_number_per_second_) {
    d_.max_send_number_per_second_ = send_num_delta;
  }
  int send_bytes_delta =
      (d_.total_send_bytes_ - d_.last_send_bytes_) / timer_interval_;
  if (send_bytes_delta > d_.max_send_bytes_per_second_) {
    d_.max_send_bytes_per_second_ = send_bytes_delta;
  }
  if (user_number > d_.max_user_number_) {
    d_.max_user_number_ = user_number;
  }
  int user_growth = user_number - d_.user_number_;
  if (user_growth > d_.max_user_growth_per_second_) {
    d_.max_user_growth_per_second_ = user_growth;
  }
  if (user_growth < d_.max_user_reduce_per_second_) {
    d_.max_user_reduce_per_second_ = user_growth;
  }

  d_.last_recv_number_ = d_.total_recv_number_;
  d_.last_recv_bytes_ = d_.total_recv_bytes_;
  d_.last_send_number_ = d_.total_send_number_;
  d_.last_send_number_ = d_.total_send_bytes_;
  d_.user_number_ = user_number;
}

void StatsManager::OnReceive(const string& data) {
  ++d_.total_recv_number_;
  d_.total_recv_bytes_ += data.size();
}

void StatsManager::OnSend(const string& data) {
  ++d_.total_send_number_;
  d_.total_send_bytes_ += data.size();
}

void StatsManager::GetReport(Json::Value& report) const {
  report["server_start_timestamp"] =
      (Json::Int64)server_start_time_.ToInternalValue();
  report["server_start_datetime"] = server_start_datetime_;
  int64 seconds = (base::Time::Now() - server_start_time_).InSeconds();
  CHECK(seconds >= 0);
  if (seconds == 0) {
    seconds = 1;
  }
  Json::Value& throughput = report["throughput"];
  throughput["total_recv_number"] = (Json::Int64)d_.total_recv_number_;
  throughput["total_recv_bytes"] = (Json::Int64)d_.total_recv_bytes_;
  throughput["total_send_number"] = (Json::Int64)d_.total_send_number_;
  throughput["total_send_bytes"] = (Json::Int64)d_.total_send_bytes_;
  throughput["avg_recv_number_per_second"] =
      (Json::Int64)d_.total_recv_number_ / seconds;
  throughput["avg_recv_bytes_per_second"] =
      (Json::Int64)d_.total_recv_bytes_ / seconds;
  throughput["avg_send_number_per_second"] =
      (Json::Int64)d_.total_send_number_ / seconds;
  throughput["avg_send_bytes_per_second"] =
      (Json::Int64)d_.total_send_bytes_ / seconds;
  throughput["max_recv_number_per_second"] =
      (Json::Int64)d_.max_recv_number_per_second_;
  throughput["max_recv_bytes_per_second"] =
      (Json::Int64)d_.max_recv_bytes_per_second_;
  throughput["max_send_number_per_second"] =
      (Json::Int64)d_.max_send_number_per_second_;
  throughput["max_send_bytes_per_second"] =
      (Json::Int64)d_.max_send_bytes_per_second_;
  throughput["user_number"] = (Json::Int64)d_.user_number_;
  throughput["max_user_number"] = (Json::Int64)d_.max_user_number_;
  throughput["max_user_growth_per_second"] =
      (Json::Int64)d_.max_user_growth_per_second_;
  throughput["max_user_reduce_per_second"] =
      (Json::Int64)d_.max_user_reduce_per_second_;
}

}  // namespace xcomet
