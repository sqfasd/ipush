#include "gtest/gtest.h"

#include <unistd.h>
#include <pthread.h>
#include <event.h>
#include "deps/base/thread.h"
#include "deps/base/time.h"
#include "deps/base/scoped_ptr.h"
#include "deps/base/callback.h"
#include "src/include_std.h"
#include "src/local_storage.h"

namespace xcomet {
class StorageUnittest : public testing::Test {
 public:
  void SaveDone(bool ok) {
    LOG(INFO) << "Storage Save Done";
    CHECK(ok);
  }

  void PopDone(string uid, MessageResult mr) {
    LOG(INFO) << "Storage PopDone: " << mr->size();
    CHECK(uid == "u1");
    CHECK(mr->size() == 3);
    CHECK(mr->at(0) == "m1");
    CHECK(mr->at(1) == "m2");
    CHECK(mr->at(2) == "m3");
  }

  void GetDone(string uid, MessageResult mr) {
    LOG(INFO) << "Storage GetDone: " << mr->size();
    CHECK(uid == "u1");
    CHECK(mr->size() == 3);
    CHECK(mr->at(0) == "m1");
    CHECK(mr->at(1) == "m2");
    CHECK(mr->at(2) == "m3");
  }

  void GetDone2(string uid, MessageResult mr) {
    LOG(INFO) << "Storage GetDone2: " << mr->size();
    CHECK(uid == "u1");
    CHECK(mr->size() == 2);
    CHECK(mr->at(0) == "m2");
    CHECK(mr->at(1) == "m3");
  }

  void GetDone3(string uid, MessageResult mr) {
    LOG(INFO) << "Storage GetDone3: " << mr->size();
    CHECK(uid == "u1");
    CHECK(mr->size() == 1);
    CHECK(mr->at(0) == "m2");
  }

  void GetDone4(string uid, MessageResult mr) {
    LOG(INFO) << "Storage GetDone4: " << mr->size();
    CHECK(uid == "u1");
    CHECK(mr->size() == 2);
    CHECK(mr->at(0) == "m2");
    CHECK(mr->at(1) == "m3");
  }
 
 protected:
  virtual void SetUp() {
    evbase_ = event_base_new();
    CHECK(evbase_) << "create event_base failed";
    storage_.reset(new LocalStorage(evbase_, "/tmp/ssdb_tmp"));

    thread_.reset(new base::ThreadRunner(
          base::NewOneTimeCallback(this, &StorageUnittest::Run)));
    thread_->Start();
  }

  virtual void TearDown() {
    storage_.reset();
    sleep(2);
    event_base_loopbreak(evbase_);
    thread_->Join();
  }

  void Run() {
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
    // StorageUnittest* self = (StorageUnittest*)user_data;
    LOG(INFO) << "OnTimer";
  }

  scoped_ptr<Storage> storage_;
  scoped_ptr<base::ThreadRunner> thread_;
  struct event_base* evbase_;
};

TEST_F(StorageUnittest, SimpleTest) {
  storage_->SaveMessage("u1", "m1",
      boost::bind(&StorageUnittest::SaveDone, this, _1));
  storage_->SaveMessage("u1", "m2",
      boost::bind(&StorageUnittest::SaveDone, this, _1));
  storage_->SaveMessage("u1", "m3",
      boost::bind(&StorageUnittest::SaveDone, this, _1));
  storage_->GetMessage("u1",
      boost::bind(&StorageUnittest::GetDone, this, "u1", _1));
  storage_->GetMessage("u1", 1, -1,
      boost::bind(&StorageUnittest::GetDone2, this, "u1", _1));
  storage_->GetMessage("u1", 1, 1,
      boost::bind(&StorageUnittest::GetDone3, this, "u1", _1));
  storage_->GetMessage("u1", 1, 2,
      boost::bind(&StorageUnittest::GetDone4, this, "u1", _1));
  storage_->GetMessage("u1", 1, 10,
      boost::bind(&StorageUnittest::GetDone4, this, "u1", _1));
  storage_->PopMessage("u1",
      boost::bind(&StorageUnittest::PopDone, this, "u1", _1));
}

}  // namespace xcomet
