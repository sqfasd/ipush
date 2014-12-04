#ifndef SRC_WORKER_H_
#define SRC_WORKER_H_

#include <event.h>
#include <boost/bind.hpp>
#include <boost/function.hpp>
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
  TaskImpl(boost::function<R ()> runner, boost::function<void (R)> cb)
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
  boost::function<R ()> runner_;
  boost::function<void (R)> callback_;
  R ret_val_;
};

template <>
class TaskImpl<void> : public Task {
 public:
  TaskImpl(boost::function<void ()> runner, boost::function<void ()> cb)
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
  boost::function<void ()> runner_;
  boost::function<void ()> callback_;
};


class Worker : public base::Thread {
 public:
  Worker(struct event_base* evbase);
  ~Worker();

  void Do(boost::function<void ()> runner, boost::function<void ()> cb) {
    size_t taskq_size = task_queue_.Size();
    if(taskq_size >= size_t(FLAGS_task_queue_warning_size)) {
      LOG(WARNING) << "taskqueue is overloaded, size: " << taskq_size;
    }
    task_queue_.Push(new TaskImpl<void>(runner, cb));
  }

  template <typename R>
  void Do(boost::function<R ()> runner, boost::function<void (R)> cb) {
    size_t taskq_size = task_queue_.Size();
    if(taskq_size >= size_t(FLAGS_task_queue_warning_size)) {
      LOG(WARNING) << "taskqueue is overloaded, size: " << taskq_size;
    }
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
