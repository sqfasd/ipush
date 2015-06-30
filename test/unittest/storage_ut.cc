#include "gtest/gtest.h"

#include "deps/base/logging.h"
#include "deps/base/file.h"
#include "deps/base/flags.h"
#include "src/include_std.h"
#include "src/storage/inmemory_storage.h"
#include "src/storage/cassandra_storage.h"
#include "test/unittest/event_loop_setup.h"

DECLARE_string(inmemory_data_dir);

namespace xcomet {
class StorageUnittest : public testing::Test {
 public:
  std::thread::id MainThreadId() {
    return event_loop_setup_->MainThreadId();
  }
 protected:
  virtual void SetUp() {
    LOG(INFO) << "SetUp";
    FLAGS_inmemory_data_dir = "/tmp/test_inmemory_data";
    event_loop_setup_ = new EventLoopSetup();
  }

  virtual void TearDown() {
    LOG(INFO) << "TearDown";
    delete event_loop_setup_;
    base::File::DeleteRecursively(FLAGS_inmemory_data_dir);
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

static std::atomic<int> counter;

static void NormalTest(Storage* s) {
  const string user = "u1";
  counter = 0;
  s->GetMessage(user, [](Error err, MessageDataSet result) {
    CHECK(err == NO_ERROR) << err;
    CHECK(result.get() == NULL || result->size() == 0);
    ++counter;
  });
  s->GetMaxSeq(user, [](Error err, int seq) {
    CHECK(err == NO_ERROR) << err;
    VLOG(3) << "seq = " << seq;
    CHECK(seq == 0);
    ++counter;
  });
  ::sleep(1);
  StringPtr msg1 = CreateMessage(1);
  int64 ttl = 5;
  s->SaveMessage(msg1, user, 1, ttl, [](Error err) {
    CHECK(err == NO_ERROR) << err;
    ++counter;
  });
  ::sleep(1);
  s->GetMaxSeq(user, [](Error err, int seq) {
    CHECK(err == NO_ERROR) << err;
    VLOG(3) << "seq = " << seq;
    CHECK(seq == 1);
    ++counter;
  });
  s->GetMessage(user, [msg1](Error err, MessageDataSet result) {
    CHECK(err == NO_ERROR) << err;
    CHECK(result.get() != NULL);
    VLOG(3) << "size = " << result->size();
    CHECK(result->size() == 1);
    CHECK_EQ(result->at(0), *msg1);
    ++counter;
  });
  ::sleep(1);

  for (int seq = 2; seq <= 10; ++seq) {
    StringPtr msg = CreateMessage(seq);
    s->SaveMessage(msg, user, seq, ttl, [](Error err) {
      CHECK(err == NO_ERROR);
      ++counter;
    });
  }

  ::sleep(1);

  s->GetMaxSeq(user, [](Error err, int seq) {
    CHECK(err == NO_ERROR);
    VLOG(3) << "seq = " << seq;
    CHECK(seq == 10);
    ++counter;
  });
  s->GetMessage(user, [msg1](Error err, MessageDataSet result) {
    CHECK(err == NO_ERROR) << err;
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
    ++counter;
  });
  ::sleep(1);

  for (int seq = 11; seq <= 150; ++seq) {
    StringPtr msg = CreateMessage(seq);
    s->SaveMessage(msg, user, seq, ttl, [](Error err) {
      CHECK(err == NO_ERROR);
      ++counter;
    });
  }

  ::sleep(1);

  s->GetMessage(user, [](Error err, MessageDataSet result) {
    CHECK(err == NO_ERROR) << err;
    CHECK(result.get() != NULL);
    CHECK(result->size() == 100) << result->size();
  });

  ::sleep(1);

  s->UpdateAck(user, 120, [](Error err) {
    CHECK(err == NO_ERROR);
    ++counter;
  });
  ::sleep(1);

  s->GetMessage(user, [msg1](Error err, MessageDataSet result) {
    CHECK(err == NO_ERROR) << err;
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
    ++counter;
  });
  ::sleep(1);

  LOG(INFO) << "waiting for message expired";
  ::sleep(4);
  s->GetMessage(user, [](Error err, MessageDataSet result) {
    CHECK(err == NO_ERROR) << err;
    CHECK(result.get() == NULL || result->size() == 0);
    ++counter;
  });

  ::sleep(1);

  CHECK(counter == 159);
}

TEST_F(StorageUnittest, InMemoryNormal) {
  InMemoryStorage storage;
  NormalTest(&storage);
}

TEST_F(StorageUnittest, CassandraNormal) {
  CassandraStorage storage;
  NormalTest(&storage);
}

}  // namespace xcomet
