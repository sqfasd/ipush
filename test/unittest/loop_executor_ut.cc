#include "gtest/gtest.h"

#include "test/unittest/event_loop_setup.h"

namespace xcomet {
class LoopExecutorUnittest : public testing::Test {
 public:
  std::thread::id MainThreadId() {
    return event_loop_setup_->MainThreadId();
  }
 protected:
  virtual void SetUp() {
    LOG(INFO) << "SetUp";
    event_loop_setup_ = new EventLoopSetup();
  }

  virtual void TearDown() {
    LOG(INFO) << "TearDown";
    delete event_loop_setup_;
  }

 private:
  EventLoopSetup* event_loop_setup_;
};

TEST_F(LoopExecutorUnittest, Normal) {
  LoopExecutor::RunInMainLoop([this]() {
    LOG(INFO) << "run in thread: " << std::this_thread::get_id();
    CHECK(std::this_thread::get_id() == MainThreadId());
  });

  auto f_add = bind([this](int a, int b) {
    LOG(INFO) << "run in thread: " << std::this_thread::get_id();
    LOG(INFO) << "a = " << a << ", b = " << b;
    CHECK(a == 3 && b == 4);
    CHECK(std::this_thread::get_id() == MainThreadId());
  }, 3, 4);
  LoopExecutor::RunInMainLoop(f_add);

  ::sleep(1);
}

}  // namespace xcomet
