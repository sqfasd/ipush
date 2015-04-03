#ifndef SRC_WORKER_H_
#define SRC_WORKER_H_

#include <event.h>
#include "deps/base/thread.h"
#include "deps/base/concurrent_queue.h"
#include "src/include_std.h"
#include "src/event_msgqueue.h"

DECLARE_int32(task_queue_warning_size);

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
  TaskImpl(function<R ()> runner, function<void (R)> cb)
      : runner_(runner), callback_(cb) {
  }

  virtual ~TaskImpl() {
  }

  virtual void Run() {
    ret_val_ = runner_();
  }

  virtual void Callback() {
    callback_(ret_val_);
  }

 private:
  function<R ()> runner_;
  function<void (R)> callback_;
  R ret_val_;
};

template <>
class TaskImpl<void> : public Task {
 public:
  TaskImpl(function<void ()> runner, function<void ()> cb)
      : runner_(runner), callback_(cb) {
  }

  virtual ~TaskImpl() {
  }

  virtual void Run() {
    runner_();
  }

  virtual void Callback() {
    callback_();
  }

 private:
  function<void ()> runner_;
  function<void ()> callback_;
};


class Worker : public base::Thread {
 public:
  Worker(struct event_base* evbase);
  ~Worker();

  void Do(function<void ()> runner, function<void ()> cb) {
    size_t taskq_size = task_queue_.Size();
    if(taskq_size >= size_t(FLAGS_task_queue_warning_size)) {
      LOG(WARNING) << "taskqueue is overloaded, size: " << taskq_size;
    }
    task_queue_.Push(new TaskImpl<void>(runner, cb));
  }

  template <typename R>
  void Do(function<R ()> runner, function<void (R)> cb) {
    size_t taskq_size = task_queue_.Size();
    if(taskq_size >= size_t(FLAGS_task_queue_warning_size)) {
      LOG(WARNING) << "taskqueue is overloaded, size: " << taskq_size;
    }
    task_queue_.Push(new TaskImpl<R>(runner, cb));
  }

  void RunInMainLoop(function<void ()> cb) {
    Do(&DoNothing, cb);
  }

 private:
  static void DoNothing() {}
  static void Callback(void* data, void* self);
  virtual void Run();
  struct event_base* evbase_;
  struct event_msgqueue* event_queue_;
  base::ConcurrentQueue<Task*> task_queue_;
};

}  // namespace xcomet
#endif  // SRC_WORKER_H_
