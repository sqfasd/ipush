
#include "base/thread.h"
#include "base/logging.h"

namespace base {

void Thread::Start() {
  pthread_attr_t attr;
  CHECK_EQ(pthread_attr_init(&attr), 0);
  CHECK_EQ(
      pthread_attr_setdetachstate(
          &attr,
          joinable_ ? PTHREAD_CREATE_JOINABLE : PTHREAD_CREATE_DETACHED),
      0);

  int result = pthread_create(&tid_, &attr, &ThreadRunner, this);
  CHECK_EQ(result, 0) << "Could not create thread (" << result << ")";

  CHECK_EQ(pthread_attr_destroy(&attr), 0);

  started_ = true;
}

void Thread::Join() {
  CHECK(joinable_) << "Thread is not joinable";
  pthread_join(tid_, NULL);
  // int result = pthread_join(tid_, NULL);
  // CHECK_EQ(result, 0) << "Could not join thread (" << result << ")";
}

}  // namespace base
