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

StringPtr CreateMessage(int seq) {
  Message msg;
  msg.SetTo("u1");
  msg.SetFrom("test");
  msg.SetBody("this is a message body");
  msg.SetType(Message::T_MESSAGE);
  msg.SetSeq(seq);
  return Message::Serialize(msg);
}

static void NormalTest(Storage* s) {
  const string user = "u1";
  s->GetMessage(user, [](ErrorPtr err, MessageDataSet result) {
    CHECK(err.get() == NULL);
    CHECK(result.get() == NULL);
  });
  s->GetMaxSeq(user, [](ErrorPtr err, int seq) {
    CHECK(err.get() == NULL);
    VLOG(3) << "seq = " << seq;
    CHECK(seq == 0);
  });
  StringPtr msg1 = CreateMessage(1);
  int64 ttl = 5;
  s->SaveMessage(msg1, user, ttl, [](ErrorPtr err) {
    CHECK(err.get() == NULL);
  });
  s->GetMaxSeq(user, [](ErrorPtr err, int seq) {
    CHECK(err.get() == NULL);
    VLOG(3) << "seq = " << seq;
    CHECK(seq == 1);
  });
  s->GetMessage(user, [msg1](ErrorPtr err, MessageDataSet result) {
    CHECK(err.get() == NULL);
    CHECK(result.get() != NULL);
    VLOG(3) << "size = " << result->size();
    CHECK(result->size() == 1);
    CHECK_EQ(result->at(0), *msg1);
  });

  for (int seq = 2; seq <= 10; ++seq) {
    StringPtr msg = CreateMessage(seq);
    s->SaveMessage(msg, user, ttl, [](ErrorPtr err) {
      CHECK(err.get() == NULL);
    });
  }

  ::sleep(1);

  s->GetMaxSeq(user, [](ErrorPtr err, int seq) {
    CHECK(err.get() == NULL);
    VLOG(3) << "seq = " << seq;
    CHECK(seq == 10);
  });
  s->GetMessage(user, [msg1](ErrorPtr err, MessageDataSet result) {
    CHECK(err.get() == NULL);
    CHECK(result.get() != NULL);
    VLOG(3) << "size = " << result->size();
    CHECK(result->size() == 10);
    for (int i = 0; i < result->size(); ++i) {
      VLOG(4) << "result " << i << ": " << result->at(i);
    }
    CHECK(result->at(0) == *msg1);
    Message msg = Message::UnserializeString(result->at(9));
    CHECK(msg.Seq() == 10);
    msg.SetSeq(1);
    CHECK(msg == Message::Unserialize(msg1));
  });

  for (int seq = 11; seq <= 150; ++seq) {
    StringPtr msg = CreateMessage(seq);
    s->SaveMessage(msg, user, ttl, [](ErrorPtr err) {
      CHECK(err.get() == NULL);
    });
  }

  ::sleep(1);

  s->UpdateAck(user, 120, [](ErrorPtr err) {
    CHECK(err.get() == NULL);
  });

  s->GetMessage(user, [msg1](ErrorPtr err, MessageDataSet result) {
    CHECK(err.get() == NULL);
    CHECK(result.get() != NULL);
    VLOG(3) << "size = " << result->size();
    CHECK(result->size() == 30);
    for (int i = 0; i < result->size(); ++i) {
      VLOG(4) << "result " << i << ": " << result->at(i);
    }
    Message msg = Message::UnserializeString(result->at(29));
    CHECK(msg.Seq() == 150);
    msg.SetSeq(1);
    CHECK(msg == Message::Unserialize(msg1));
  });

  LOG(INFO) << "waiting for message expired";
  ::sleep(4);
  s->GetMessage(user, [](ErrorPtr err, MessageDataSet result) {
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
