#ifndef TEST_UNITTEST_EVENT_LOOP_SETUP_H_
#define TEST_UNITTEST_EVENT_LOOP_SETUP_H_

#include <event.h>
#include "src/include_std.h"
#include "src/loop_executor.h"

namespace xcomet {
class EventLoopSetup {
 public:
  EventLoopSetup() {
    evbase_ = event_base_new();
    LoopExecutor::Init(evbase_);
    CHECK(evbase_) << "create event_base failed";
    LOG(INFO) << "event base created";
    main_thread_ = std::thread(&EventLoopSetup::MainLoop, this);
    LOG(INFO) << "start event loop";
  }

  ~EventLoopSetup() {
    LOG(INFO) << "~EventLoopSetup";
    LoopExecutor::Destroy();
    int ret = event_base_loopbreak(evbase_);
    if (main_thread_.joinable()) {
      LOG(INFO) << "join main thread";
      main_thread_.join();
    }
    LOG(INFO) << "finished";
  }

  std::thread::id MainThreadId() {
    return main_thread_id_;
  }

  struct event_base* EvBase() {
    return evbase_;
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
 private:
  struct event_base* evbase_;
  std::thread main_thread_;
  std::thread::id main_thread_id_;
};
}  // namespace xcomet

#endif  // TEST_UNITTEST_EVENT_LOOP_SETUP_H_
