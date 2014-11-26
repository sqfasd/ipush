#ifndef SRC_WORKER_H_
#define SRC_WORKER_H_

#include <event.h>
#include "deps/base/callback.h"
#include "deps/base/thread.h"
#include "deps/base/concurrent_queue.h"
#include "src/include_std.h"
#include "src/event_msgqueue.h"

namespace xcomet {

class Task {
 public:
  virtual ~Task() {}
  virtual void Run() = 0;
  virtual void Callback() = 0;
};

template <typename R>
class TaskImpl : public Task {
 public:
  TaskImpl(base::ResultCallback<R>* runner, base::Callback1<R>* cb)
      : runner_(runner), callback_(cb) {
  }

  virtual ~TaskImpl() {
  }

  virtual void Run() {
    ret_val_ = runner_->Run();
  }

  virtual void Callback() {
    callback_->Run(ret_val_);
  }

 private:
  base::ResultCallback<R>* runner_;
  base::Callback1<R>* callback_;
  R ret_val_;
};

template <>
class TaskImpl<void> : public Task {
 public:
  TaskImpl(base::Closure* runner, base::Closure* cb) 
      : runner_(runner), callback_(cb) {
  }

  virtual ~TaskImpl() {
  }

  virtual void Run() {
    runner_->Run();
  }

  virtual void Callback() {
    callback_->Run();
  }

 private:
  base::Closure* runner_;
  base::Closure* callback_;
};


class Worker : public base::Thread {
 public:
  Worker(struct event_base* evbase);
  ~Worker();

  void Do(base::Closure* runner, base::Closure* cb) {
    task_queue_.Push(new TaskImpl<void>(runner, cb));
  }

  template <typename R>
  void Do(base::ResultCallback<R>* runner, base::Callback1<R>* cb) {
    task_queue_.Push(new TaskImpl<R>(runner, cb));
  }

  virtual void Run();
 private:
  static void RunInEventloop(void* data, void* self);
  struct event_base* evbase_;
  struct event_msgqueue* event_queue_;
  base::ConcurrentQueue<Task*> task_queue_;
};

}  // namespace xcomet
#endif  // SRC_WORKER_H_
