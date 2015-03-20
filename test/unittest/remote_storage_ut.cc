#include "gtest/gtest.h"

#include <unistd.h>
#include <pthread.h>
#include <event.h>
#include "deps/base/thread.h"
#include "deps/base/time.h"
#include "deps/base/scoped_ptr.h"
#include "deps/base/callback.h"
#include "src/include_std.h"
#include "src/remote_storage.h"

namespace xcomet {
class RemoteStorageUnittest : public testing::Test {
 public:
  static void CheckSeq(int expect_seq, int seq) {
    LOG(INFO) << "CheckSeq";
    CHECK(expect_seq == seq);
  }

  static void CheckResult1(MessageResult mr) {
    LOG(INFO) << "CheckResult1";
    CHECK(mr.get());
    CHECK(mr->size() == 0);
  }

  static void CheckOK(bool ok) {
    LOG(INFO) << "CheckOK";
    CHECK(ok);
  }

  static void CheckResult2(MessageResult mr) {
    LOG(INFO) << "CheckResult2";
    CHECK(mr.get());
    CHECK(mr->size() == 2);
    Json::Value value;
    Json::Reader reader;
    CHECK(mr->at(0) == "1");
    CHECK(reader.parse(mr->at(1), value));
    CHECK(value["from"] == "u:1");
    CHECK(value["type"] == "msg");
    CHECK(value["body"] == "msg_body_1");
  }

  static void CheckResult3(MessageResult mr) {
    LOG(INFO) << "CheckResult3";
    CHECK(mr.get());
    CHECK(mr->size() == 6);
    CHECK(mr->at(0) == "1");
    CHECK(mr->at(2) == "2");
    CHECK(mr->at(4) == "3");

    Json::Reader reader;
    Json::Value v1;
    CHECK(reader.parse(mr->at(1), v1));
    CHECK(v1["from"] == "u:1");
    CHECK(v1["type"] == "msg");
    CHECK(v1["body"] == "msg_body_1");

    Json::Value v2;
    CHECK(reader.parse(mr->at(3), v2));
    CHECK(v2["from"] == "u:1");
    CHECK(v2["type"] == "msg");
    CHECK(v2["body"] == "msg_body_2");

    Json::Value v3;
    CHECK(reader.parse(mr->at(5), v3));
    CHECK(v3["from"] == "u:1");
    CHECK(v3["type"] == "msg");
    CHECK(v3["body"] == "msg_body_3");
  }

  static void CheckResult4(MessageResult mr) {
    LOG(INFO) << "CheckResult4";
    CHECK(mr.get());
    CHECK(mr->size() == 4);
    CHECK(mr->at(0) == "2");
    CHECK(mr->at(2) == "3");

    Json::Reader reader;
    Json::Value v1;
    CHECK(reader.parse(mr->at(1), v1));
    CHECK(v1["from"] == "u:1");
    CHECK(v1["type"] == "msg");
    CHECK(v1["body"] == "msg_body_2");

    Json::Value v2;
    CHECK(reader.parse(mr->at(3), v2));
    CHECK(v2["from"] == "u:1");
    CHECK(v2["type"] == "msg");
    CHECK(v2["body"] == "msg_body_3");
  }

  static void CheckResult5(MessageResult mr) {
    LOG(INFO) << "CheckResult5";
    CHECK(mr.get());
    CHECK(mr->size() == 0);
  }

  static void CheckResult6(MessageResult mr) {
    LOG(INFO) << "CheckResult6";
    CHECK(mr.get());
    CHECK(mr->size() == 2);
    CHECK(mr->at(0) == "4");
    Json::Reader reader;
    Json::Value v1;
    CHECK(reader.parse(mr->at(1), v1));
    CHECK(v1["from"] == "u:1");
    CHECK(v1["type"] == "msg");
    CHECK(v1["body"] == "msg_body_4");
  }

 protected:
  virtual void SetUp() {
    evbase_ = event_base_new();
    CHECK(evbase_) << "create event_base failed";
    storage_.reset(new RemoteStorage(evbase_));

    thread_.reset(new base::ThreadRunner(
          base::NewOneTimeCallback(this, &RemoteStorageUnittest::Run)));
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

TEST_F(RemoteStorageUnittest, SimpleTest) {
  storage_->GetMaxSeq("u:1",
      boost::bind(&CheckSeq, 0, _1));
  storage_->GetMessage("u:1",
      boost::bind(&CheckResult1, _1));

  MessagePtr msg1(new Json::Value());
  (*msg1)["from"] = "u:1";
  (*msg1)["type"] = "msg";
  (*msg1)["body"] = "msg_body_1";
  storage_->SaveMessage(msg1, 0,
      boost::bind(&CheckOK, _1));
  storage_->GetMessage("u:1",
      boost::bind(&CheckResult2, _1));
  storage_->GetMaxSeq("u:1",
      boost::bind(&CheckSeq, 1, _1));

  MessagePtr msg2(new Json::Value());
  (*msg2)["from"] = "u:1";
  (*msg2)["type"] = "msg";
  (*msg2)["body"] = "msg_body_2";
  storage_->SaveMessage(msg2, 0,
      boost::bind(&CheckOK, _1));
  MessagePtr msg3(new Json::Value());
  (*msg3)["from"] = "u:1";
  (*msg3)["type"] = "msg";
  (*msg3)["body"] = "msg_body_3";
  storage_->SaveMessage(msg3, 0,
      boost::bind(&CheckOK, _1));
  storage_->GetMessage("u:1",
      boost::bind(&CheckResult3, _1));
  storage_->GetMaxSeq("u:1",
      boost::bind(&CheckSeq, 3, _1));

  storage_->UpdateAck("u:1", 1,
      boost::bind(&CheckOK, _1));
  storage_->GetMessage("u:1",
      boost::bind(&CheckResult4, _1));

  storage_->UpdateAck("u:1", 3,
      boost::bind(&CheckOK, _1));
  storage_->GetMessage("u:1",
      boost::bind(&CheckResult5, _1));

  MessagePtr msg4(new Json::Value());
  (*msg4)["from"] = "u:1";
  (*msg4)["type"] = "msg";
  (*msg4)["body"] = "msg_body_4";
  storage_->SaveMessage(msg4, 0,
      boost::bind(&CheckOK, _1));
  storage_->GetMessage("u:1",
      boost::bind(&CheckResult6, _1));
  storage_->GetMaxSeq("u:1",
      boost::bind(&CheckSeq, 4, _1));

  sleep(1);
}

}  // namespace xcomet
