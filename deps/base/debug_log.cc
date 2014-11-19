#include "base/debug_log.h"

namespace base {

__thread DebugLog* DebugLog::thread_instance_ = NULL;

}  // namespace base
