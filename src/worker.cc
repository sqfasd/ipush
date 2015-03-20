#include "src/worker.h"


DEFINE_int32(msgqueue_max_size, 20000, "");
DEFINE_int32(task_queue_warning_size, 10000, "");

namespace xcomet {

Worker::Worker(struct event_base* evbase)
    : base::Thread(true), // joinable
      evbase_(evbase),
      event_queue_(NULL) {
  CHECK(evbase_);
  event_queue_ = msgqueue_new(evbase_,
                              FLAGS_msgqueue_max_size,
                              &RunInEventloop,
                              this);
  CHECK(event_queue_) << "failed to create event_msgqueue";
  Start();
}

Worker::~Worker() {
  task_queue_.Push(NULL);
  LOG(INFO) << "Worker destroy, waiting for task thread exit";
  this->Join();
  msgqueue_destroy(event_queue_);
}

void Worker::RunInEventloop(void* data, void* self) {
  Task* task = (Task*)data;
  task->Callback();
  delete task;
}

void Worker::Run() {
  while (true) {
    VLOG(5) << "task queue size:" <<  task_queue_.Size();
    Task* task;
    task_queue_.Pop(task);
    if (task == NULL) {
      break;
    }
    VLOG(3) << "before run task";
    task->Run();
    VLOG(3) << "after run task";
    int ret = msgqueue_push(event_queue_, task);
    if (ret == -1) {
      LOG(ERROR) << "msgqueue_push failed, callback would not execute";
    }
  }
  LOG(INFO) << "Worker thread exit";
}

}  // namespace xcomet
