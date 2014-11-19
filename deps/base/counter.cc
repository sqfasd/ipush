
#include "base/counter.h"
#include "base/string_util.h"
#include "base/yr.h"

namespace base {

namespace counter {

vector<pair<string, uint64*> > counters;
bool enable_counters = false;

const std::string PrintCounters() {
  std::string str;
  if (enable_counters) {
    for (int i = 0; i < counters.size(); ++i)
      str += StringPrintf("\n\t%s: %ld",
          counters[i].first.c_str(),
          *counters[i].second);
  } else {
    str = "counters is disabled";
  }

  return str;
}

void ResetCounters() {
  for (int i = 0; i < counters.size(); ++i) {
    Counter(counters[i].second).Reset();
  }
}

void EnableCounters() {
  enable_counters = true;
}

void DisableCounters() {
  enable_counters = false;
}

}  // namespace counter

Counter::Counter(uint64 *counter) : v_(counter) {}

void Counter::Reset() {
  AtomicPointerAssgin(v_, 0UL);
}

uint64 Counter::value() const {
  __sync_synchronize();
  return *v_;
}

}  // namespace base
