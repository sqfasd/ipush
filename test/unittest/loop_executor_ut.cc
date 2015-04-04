#include "gtest/gtest.h"

#include <event.h>
#include "src/include_std.h"
#include "src/loop_executor.h"

namespace xcomet {
class LoopExecutorUnittest : public testing::Test {
 protected:
  virtual void SetUp() {
    LOG(INFO) << "SetUp";
    evbase_ = event_base_new();
    LoopExecutor::Init(evbase_);
    CHECK(evbase_) << "create event_base failed";
    LOG(INFO) << "event base created";
    main_thread_ = std::thread(&LoopExecutorUnittest::MainLoop, this);
    LOG(INFO) << "start event loop";
  }

  virtual void TearDown() {
    LOG(INFO) << "TearDown";
    LoopExecutor::Destroy();
    int ret = event_base_loopbreak(evbase_);
    if (main_thread_.joinable()) {
      LOG(INFO) << "join main thread";
      main_thread_.join();
    }
    LOG(INFO) << "finished";
  }

  void MainLoop() {
    main_thread_id_ = std::this_thread::get_id();
    LOG(INFO) << "eventloop thread id: " << main_thread_id_;

    struct event* evtimer = NULL;
    evtimer = event_new(evbase_, -1, EV_PERSIST, OnTimer, this);
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    CHECK(evtimer && evtimer_add(evtimer, &tv) == 0)
        << "create timer event failed";

    event_base_dispatch(evbase_);
    LOG(INFO) << "event loop exited";
  }

  static void OnTimer(evutil_socket_t sig, short events, void* user_data) {
    LOG(INFO) << "OnTimer";
  }
  
  struct event_base* evbase_;
  std::thread main_thread_;
  std::thread::id main_thread_id_;
};

TEST_F(LoopExecutorUnittest, Normal) {
  LoopExecutor::RunInMainLoop([this]() {
    LOG(INFO) << "run in thread: " << std::this_thread::get_id();
    CHECK(std::this_thread::get_id() == main_thread_id_);
  });

  auto f_add = bind([this](int a, int b) {
    LOG(INFO) << "run in thread: " << std::this_thread::get_id();
    LOG(INFO) << "a = " << a << ", b = " << b;
    CHECK(a == 3 && b == 4);
    CHECK(std::this_thread::get_id() == main_thread_id_);
  }, 3, 4);
  LoopExecutor::RunInMainLoop(f_add);

  ::sleep(1);
}

}  // namespace xcomet
