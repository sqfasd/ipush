#include "src/worker.h"
#include "deps/base/time.h"

DEFINE_int32(msgqueue_max_size, 200000, "");
DEFINE_int32(task_queue_warning_size, 100000, "");

namespace xcomet {

Worker::Worker(struct event_base* evbase)
    : base::Thread(true), // joinable
      evbase_(evbase),
      event_queue_(NULL) {
  CHECK(evbase_);
  event_queue_ = msgqueue_new(evbase_,
                              FLAGS_msgqueue_max_size,
                              &Callback,
                              this);
  CHECK(event_queue_) << "failed to create event_msgqueue";
  Start();
}

Worker::~Worker() {
  task_queue_.Push(NULL);
  LOG(INFO) << "Worker destroy, waiting for task thread exit";
  this->Join();
  LOG(INFO) << "worker thread exited";
  msgqueue_destroy(event_queue_);
  LOG(INFO) << "msgqueue destroyed";
}

void Worker::Callback(void* data, void* self) {
  Task* task = (Task*)data;
  task->Callback();
  delete task;
}

void Worker::Run() {
  while (true) {
    VLOG(6) << "task queue size:" <<  task_queue_.Size();
    if (task_queue_.Empty()) {
      base::MilliSleep(100); 
      continue;
    }
    Task* task = task_queue_.Front();
    if (task == NULL) {
      break;
    }
    VLOG(6) << "before run task";
    task->Run();
    VLOG(6) << "after run task";
    int ret = msgqueue_push(event_queue_, task);
    if (ret == -1) {
      LOG(WARNING) << "msgqueue_push failed, callback would not execute";
      base::MilliSleep(100); 
    } else {
      task_queue_.Pop(task);
    }
  }
  LOG(INFO) << "Worker thread exit";
}

}  // namespace xcomet
