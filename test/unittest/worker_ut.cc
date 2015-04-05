#include "gtest/gtest.h"

#include "deps/base/callback.h"
#include "src/include_std.h"
#include "src/worker.h"
#include "test/unittest/event_loop_setup.h"

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
    CHECK_EQ(MainThreadId(), std::this_thread::get_id());
  }

  void Callback1(int* ret) {
    LOG(INFO) << "SlowWork1 done, come back now";
    CHECK_EQ(MainThreadId(), std::this_thread::get_id());
    CHECK_EQ(*ret, 1);
    delete ret;
  }

  std::thread::id MainThreadId() {
    return event_loop_setup_->MainThreadId();
  }
 
 protected:
  virtual void SetUp() {
    event_loop_setup_ = new EventLoopSetup();
    worker_.reset(new Worker(event_loop_setup_->EvBase()));
  }

  virtual void TearDown() {
    worker_.reset();
    delete event_loop_setup_;
  }

  shared_ptr<Worker> worker_;
  EventLoopSetup* event_loop_setup_;
};

TEST_F(WorkerUnittest, SimpleTest) {
  worker_->Do(
      bind(&WorkerUnittest::SlowWork, this),
      bind(&WorkerUnittest::Callback, this));
  worker_->Do<int*>(
      bind(&WorkerUnittest::SlowWork1, this),
      bind(&WorkerUnittest::Callback1, this, _1));
}

}  // namespace xcomet
