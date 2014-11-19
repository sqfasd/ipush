//
#include "base/affinity.h"
#include <unistd.h>

#include <unistd.h>

#include <sys/sysinfo.h>
#include <sched.h>
#include <string>
#include <vector>

#include "base/flags.h"
#include "base/string_util.h"

DEFINE_string(
    affinity_setting,
    "",
    "Affinity setting: specify the cpu ids, seperated by comma.E.g.: '0,1'");

namespace base {
  void ApplyAffinitySetting() {
    if (FLAGS_affinity_setting.empty()) {
      return;
    }
    std::vector<std::string> results;
    SplitString(FLAGS_affinity_setting, ',', &results);
    int cpu_num = sysconf(_SC_NPROCESSORS_CONF);

    cpu_set_t mask;
    CPU_ZERO(&mask);
    for (int i = 0; i < results.size(); i++) {
      int cpu_id = StringToInt(results[i]);
      CHECK_LT(cpu_id, cpu_num);
      CPU_SET(cpu_id, &mask);
    }
    LOG(INFO) << "Set CPU affinity " << FLAGS_affinity_setting;
    if (sched_setaffinity(0, sizeof(mask), &mask)) {
      LOG(WARNING) << "Could not set CPU affinity " << FLAGS_affinity_setting;
    }
  }
}  // namespace base
