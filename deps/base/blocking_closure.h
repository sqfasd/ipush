
#ifndef BASE_BLOCKING_CLOSURE_H_
#define BASE_BLOCKING_CLOSURE_H_

#include "base/callback.h"
#include "base/mutex.h"

namespace base {
class BlockingClosure : public Closure {
 public:
  explicit BlockingClosure(Closure* cb) : cb_(cb), done_(false) {};
  BlockingClosure() : cb_(NULL), done_(false) {};
  void Run() {
    if (cb_ != NULL) {
      cb_->Run();
    }
    MutexLock l(&mutex_);
    done_ = true;
    cond_.SignalAll();
  }
  void Wait() {
    MutexLock lock(&mutex_);
    while(!done_) {
      cond_.Wait(&mutex_);
    }
  }
  void Reset() {
    MutexLock lock(&mutex_);
    done_ = false;
  }

 private:
  Closure* cb_;
  Mutex mutex_;
  bool done_;
  CondVar cond_;
};
}  // namespace base
#endif  // BASE_BLOCKING_CLOSURE_H_
