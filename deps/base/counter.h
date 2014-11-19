
#ifndef BASE_COUNTER_H_
#define BASE_COUNTER_H_

#include <string>
#include <vector>
#include <utility>

#include "base/atomic.h"
#include "base/basictypes.h"
#include "base/flags.h"

namespace base {

namespace counter {

extern std::vector<std::pair<std::string, uint64*> > counters;
extern bool enable_counters;

const std::string PrintCounters();
void ResetCounters();
void EnableCounters();
void DisableCounters();

}  // namespace counter

// An counter class that could be incremented atomically.
// see counter_test.cc for usage.
class Counter {
 public:
  Counter(uint64 *counter);  // NOLINT

  void Reset();

  void Increment() {
    if (::base::counter::enable_counters) {
      AtomicIncrement(v_);
    }
  }

  inline void IncrementBy(uint64 value) {
    if (::base::counter::enable_counters) {
      AtomicIncrement(v_, value);
    }
  }

  uint64 value() const;

 private:
  uint64 *v_;
  DISALLOW_COPY_AND_ASSIGN(Counter);
};

}  // namespace base

#define REGISTER_COUNTER(name)          \
  namespace yr_counter {                \
  static uint64 COUNTER_ ## name;       \
  void register_counter_##name() {      \
    ::base::counter::counters.push_back(\
        std::make_pair(#name, &COUNTER_ ## name)); \
  } \
  __attribute__((constructor)) void register_counter_##name(); \
  }                                     \
  using yr_counter::COUNTER_##name

#define DECLARE_COUNTER(name)           \
  namespace yr_counter {                \
    extern uint64 COUNTER_ ## name;     \
  }                                     \
  using yr_counter::COUNTER_##name

#define COUNTER(name)                   \
  base::Counter(reinterpret_cast<uint64*>(\
        &yr_counter::COUNTER_ ## name))

#endif  // BASE_COUNTER_H_
