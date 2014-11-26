#include "gtest/gtest.h"

#include <unistd.h>
#include <pthread.h>
#include <event.h>
#include "deps/base/thread.h"
#include "deps/base/scoped_ptr.h"
#include "deps/base/callback.h"
#include "src/include_std.h"
#include "src/worker.h"

namespace xcomet {
class WorkerUnittest : public testing::Test {
 public:
  void SlowWork() {
    LOG(INFO) << "Doing SlowWork that cost 1 seconds";
    sleep(1);
  }

  int* SlowWork1() {
    LOG(INFO) << "Doing SlowWork1 that cost 1 seconds";
    sleep(1);
    int* i = new int(1);
    return i;
  }

  void Callback() {
    LOG(INFO) << "SlowWork done, come back now";
    CHECK_EQ(pid_, pthread_self());
  }

  void Callback1(int* ret) {
    LOG(INFO) << "SlowWork1 done, come back now";
    CHECK_EQ(pid_, pthread_self());
    CHECK_EQ(*ret, 1);
    delete ret;
  }
 
 protected:
  virtual void SetUp() {
    evbase_ = event_base_new();
    CHECK(evbase_) << "create event_base failed";
    worker_.reset(new Worker(evbase_));

    thread_.reset(new base::ThreadRunner(
          base::NewOneTimeCallback(this, &WorkerUnittest::Run)));
    thread_->Start();
  }

  virtual void TearDown() {
    worker_.reset();
    event_base_loopbreak(evbase_);
    thread_->Join();
  }

  void Run() {
    pid_ = pthread_self();
    LOG(INFO) << "eventloop thread id: " << pthread_self();

    struct event* evtimer = NULL;
    evtimer = event_new(evbase_, -1, EV_PERSIST, OnTimer, this);
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    CHECK(evtimer && evtimer_add(evtimer, &tv) == 0)
        << "create timer event failed";

    event_base_dispatch(evbase_);
  }
  
  static void OnTimer(evutil_socket_t sig, short events, void* user_data) {
    // WorkerUnittest* self = (WorkerUnittest*)user_data;
    LOG(INFO) << "OnTimer";
  }

  scoped_ptr<Worker> worker_;
  scoped_ptr<base::ThreadRunner> thread_;
  struct event_base* evbase_;
  pthread_t pid_;
  int* ret_;
};

TEST_F(WorkerUnittest, SimpleTest) {
  worker_->Do(
      base::NewOneTimeCallback(this, &WorkerUnittest::SlowWork),
      base::NewOneTimeCallback(this, &WorkerUnittest::Callback));
  worker_->Do(
      base::NewOneTimeCallback(this, &WorkerUnittest::SlowWork1),
      base::NewOneTimeCallback(this, &WorkerUnittest::Callback1));
}

}  // namespace xcomet
