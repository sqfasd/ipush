#ifndef SRC_LOOP_EXECUTOR_H_
#define SRC_LOOP_EXECUTOR_H_

#include "deps/base/singleton.h"
#include "src/include_std.h"

namespace xcomet {

class LoopExecutorImpl;
class LoopExecutor {
 public:
  static LoopExecutor& Instance() {
    return *Singleton<LoopExecutor>::get();
  }
  static void RunInMainLoop(function<void ()> f) {
    return Instance().DoRunInMainLoop(f);
  }
  static void Destroy() {
    Instance().DoDestroy();
  }
  static void Init(void* loop_base) {
    Instance().DoInit(loop_base);
  }

 private:
  LoopExecutor();
  ~LoopExecutor();
  void DoInit(void* loop_base);
  void DoDestroy();
  void DoRunInMainLoop(function<void ()> f);

  static void Callback(void* data, void* ctx);

  LoopExecutorImpl* p_;
  DISALLOW_COPY_AND_ASSIGN(LoopExecutor);

  friend struct DefaultSingletonTraits<LoopExecutor>;
};

}  // namespace xcomet
#endif  // SRC_LOOP_EXECUTOR_H_
