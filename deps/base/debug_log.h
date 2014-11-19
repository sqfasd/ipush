#ifndef BASE_DEBUG_LOG_H_
#define BASE_DEBUG_LOG_H_

#include <string>
#include <sstream>

namespace base {

using std::string;
using std::ostringstream;
using std::ostream;

#define DEBUG_LOG_INIT(level, target)\
  DebugLog __debug_log__(level, target);

#define DEBUG_LOG(level)\
  (DebugLog::ThreadInstance() != NULL) &&\
  ((level) <=  DebugLog::ThreadInstance()->Level()) &&\
  (DebugLog::ThreadInstance()->Stream())

class DebugLog {
 public:
  DebugLog(int level, string& target)
      : level_(level), target_(target) {
    SetThreadInstance(this);
  }
  ~DebugLog() {
    target_ += stream_.str();
    SetThreadInstance(NULL);
  }

  std::ostream& Stream() {
    return stream_;
  }

  int Level() {
    return level_;
  }

  static DebugLog* ThreadInstance() {
    return thread_instance_;
  }

  static void SetThreadInstance(DebugLog* debug_log) {
    thread_instance_ = debug_log;
  }
  
 private:
  static __thread DebugLog* thread_instance_;

  std::ostringstream stream_;
  int level_;
  string& target_;
};

}  // namespace base

#endif  // BASE_DEBUG_LOG_H_
