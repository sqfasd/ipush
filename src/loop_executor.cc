#include "src/loop_executor.h"

#include <event.h>
#include "deps/base/logging.h"
#include "src/event_msgqueue.h"

namespace xcomet {

const int DEFAULT_CALLBACK_QUEUE_SIZE = 1000000;

struct LoopExecutorImpl {
  struct event_msgqueue* callback_queue;
};

LoopExecutor::LoopExecutor() {
}

LoopExecutor::~LoopExecutor() {
}

void LoopExecutor::DoDestroy() {
  if (p_->callback_queue) {
    msgqueue_destroy(p_->callback_queue);
  }
  delete p_;
}

void LoopExecutor::Callback(void* data, void* ctx) {
  function<void ()>* cb = static_cast<function<void ()>*>(data); 
  (*cb)();
  delete cb;
}


void LoopExecutor::DoInit(void* loop_base) {
  p_ = new LoopExecutorImpl();
  struct event_base* evbase = static_cast<event_base*>(loop_base);
  p_->callback_queue = msgqueue_new(evbase,
                                    DEFAULT_CALLBACK_QUEUE_SIZE,
                                    &Callback,
                                    NULL);
  CHECK(p_->callback_queue) << "failed to create callback queue";
}

void LoopExecutor::DoRunInMainLoop(function<void ()> f) {
  function<void ()>* cb = new function<void ()>(f);
  while (true) {
    if (msgqueue_push(p_->callback_queue, cb) == -1) {
      LOG(WARNING) << "msgqueue_push failed, will try 100ms later";
      base::MilliSleep(100);
    } else {
      break;
    }
  }
}

}
