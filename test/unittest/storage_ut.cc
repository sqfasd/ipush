#include "gtest/gtest.h"

#include "deps/base/logging.h"
#include "src/include_std.h"
#include "src/inmemory_storage.h"
#include "test/unittest/event_loop_setup.h"

namespace xcomet {
class StorageUnittest : public testing::Test {
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

MessagePtr CreateMessage(int seq) {
  MessagePtr msg(new Json::Value());
  (*msg)["to"] = "u1";
  (*msg)["from"] = "test";
  (*msg)["body"] = "this is a message body";
  (*msg)["type"] = "msg";
  (*msg)["seq"] = seq;
  return msg;
}

static void NormalTest(Storage* s) {
  s->GetMessage("u1", [](ErrorPtr err, MessageDataSet result) {
    CHECK(err.get() == NULL);
    CHECK(result.get() == NULL);
  });
  s->GetMaxSeq("u1", [](ErrorPtr err, int seq) {
    CHECK(err.get() == NULL);
    VLOG(3) << "seq = " << seq;
    CHECK(seq == 0);
  });
  MessagePtr msg1 = CreateMessage(1);
  int64 ttl = 5;
  s->SaveMessage(msg1, ttl, [](ErrorPtr err) {
    CHECK(err.get() == NULL);
  });
  s->GetMaxSeq("u1", [](ErrorPtr err, int seq) {
    CHECK(err.get() == NULL);
    VLOG(3) << "seq = " << seq;
    CHECK(seq == 1);
  });
  s->GetMessage("u1", [msg1](ErrorPtr err, MessageDataSet result) {
    CHECK(err.get() == NULL);
    CHECK(result.get() != NULL);
    VLOG(3) << "size = " << result->size();
    CHECK(result->size() == 1);
    CHECK_EQ(result->at(0), *(Message::Serialize(msg1)));
  });

  for (int seq = 2; seq <= 10; ++seq) {
    MessagePtr msg = CreateMessage(seq);
    s->SaveMessage(msg, ttl, [](ErrorPtr err) {
      CHECK(err.get() == NULL);
    });
  }

  ::sleep(1);

  s->GetMaxSeq("u1", [](ErrorPtr err, int seq) {
    CHECK(err.get() == NULL);
    VLOG(3) << "seq = " << seq;
    CHECK(seq == 10);
  });
  s->GetMessage("u1", [msg1](ErrorPtr err, MessageDataSet result) {
    CHECK(err.get() == NULL);
    CHECK(result.get() != NULL);
    VLOG(3) << "size = " << result->size();
    CHECK(result->size() == 10);
    for (int i = 0; i < result->size(); ++i) {
      VLOG(4) << "result " << i << ": " << result->at(i);
    }
    CHECK(result->at(0) == *(Message::Serialize(msg1)));
    MessagePtr msg = Message::UnserializeString(result->at(9));
    CHECK((*msg)["seq"].asInt() == 10);
    (*msg)["seq"] = 1;
    CHECK(*msg == *msg1);
  });

  for (int seq = 11; seq <= 150; ++seq) {
    MessagePtr msg = CreateMessage(seq);
    s->SaveMessage(msg, ttl, [](ErrorPtr err) {
      CHECK(err.get() == NULL);
    });
  }

  ::sleep(1);

  s->UpdateAck("u1", 120, [](ErrorPtr err) {
    CHECK(err.get() == NULL);
  });

  s->GetMessage("u1", [msg1](ErrorPtr err, MessageDataSet result) {
    CHECK(err.get() == NULL);
    CHECK(result.get() != NULL);
    VLOG(3) << "size = " << result->size();
    CHECK(result->size() == 30);
    for (int i = 0; i < result->size(); ++i) {
      VLOG(4) << "result " << i << ": " << result->at(i);
    }
    MessagePtr msg = Message::UnserializeString(result->at(29));
    CHECK((*msg)["seq"].asInt() == 150);
    (*msg)["seq"] = 1;
    CHECK(*msg == *msg1);
  });

  LOG(INFO) << "waiting for message expired";
  ::sleep(4);
  s->GetMessage("u1", [](ErrorPtr err, MessageDataSet result) {
    CHECK(err.get() == NULL);
    CHECK(result.get() != NULL);
    CHECK(result->size() == 0);
  });

  ::sleep(1);
}

TEST_F(StorageUnittest, InMemoryNormal) {
  InMemoryStorage storage;
  NormalTest(&storage);
}

//TEST_F(StorageUnittest, CassandraNormal) {
//  CassandraStorage storage;
//  NormalTest(&storage);
//}

}  // namespace xcomet
