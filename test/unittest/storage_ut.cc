#include "gtest/gtest.h"

#include <unistd.h>
#include <pthread.h>
#include <event.h>
#include "deps/base/thread.h"
#include "deps/base/time.h"
#include "deps/base/scoped_ptr.h"
#include "deps/base/callback.h"
#include "src/include_std.h"
#include "src/storage.h"

namespace xcomet {
class StorageUnittest : public testing::Test {
 public:
  void SaveDone(bool ok) {
    CHECK(ok);
    LOG(INFO) << "Storage Save Done ok";
  }

  void GetDone(MessageIteratorPtr mit) {
    LOG(INFO) << "Storage Get Done ok";
    CHECK(mit->HasNext());
    CHECK(mit->Next() == "m1");
    CHECK(mit->HasNext());
    CHECK(mit->Next() == "m2");
    CHECK(mit->HasNext());
    CHECK(mit->Next() == "m3");
    CHECK(!mit->HasNext());
  }
 
 protected:
  virtual void SetUp() {
    evbase_ = event_base_new();
    CHECK(evbase_) << "create event_base failed";
    storage_.reset(new Storage(evbase_, "/tmp/ssdb_tmp"));

    thread_.reset(new base::ThreadRunner(
          base::NewOneTimeCallback(this, &StorageUnittest::Run)));
    thread_->Start();
  }

  virtual void TearDown() {
    storage_.reset();
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
  storage_->SaveOfflineMessage("u1", "m1",
      base::NewOneTimeCallback(this, &StorageUnittest::SaveDone));
  storage_->SaveOfflineMessage("u1", "m2",
      base::NewOneTimeCallback(this, &StorageUnittest::SaveDone));
  storage_->SaveOfflineMessage("u1", "m3",
      base::NewOneTimeCallback(this, &StorageUnittest::SaveDone));
  storage_->GetOfflineMessageIterator("u1",
      base::NewOneTimeCallback(this, &StorageUnittest::GetDone));
}

}  // namespace xcomet