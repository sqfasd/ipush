#include "gtest/gtest.h"

#include "deps/base/logging.h"
#include "src/message.h"

namespace xcomet {

TEST(MessageUnittest, Unserialize) {
  const char* json = "{\"t\": \"user1\",\"f\":\"test\","
                     "\"s\":1,  \"y\":3,\"b\":\"this is a message\"}";
  Message msg1 = Message::UnserializeString(json);
  LOG(INFO) << msg1;
  CHECK(msg1.HasSeq());
  CHECK(msg1.HasTo());
  CHECK(msg1.HasType());
  CHECK(msg1.To() == "user1");
  CHECK(msg1.From() == "test");
  CHECK(msg1.Seq() == 1);
  CHECK(msg1.Type() == Message::T_MESSAGE);
  CHECK(msg1.Body() == "this is a message");

  StringPtr str = Message::Serialize(msg1);
  Message msg2 = Message::Unserialize(str);
  CHECK(msg1 == msg2);
}

TEST(MessageUnittest, UnserializeEmbed) {
  const char* json = "{\"t\": \"user1\",\"f\":\"test\","
                     "\"s\":1,  \"y\":3,\"b\":\""
                     "{\\\"content\\\":\\\"embedded json body\\\"}"
                     "\"}";
  Message msg1 = Message::UnserializeString(json);
  LOG(INFO) << msg1;
  CHECK(msg1.HasSeq());
  CHECK(msg1.HasTo());
  CHECK(msg1.HasType());
  CHECK(msg1.To() == "user1");
  CHECK(msg1.From() == "test");
  CHECK(msg1.Seq() == 1);
  CHECK(msg1.Type() == Message::T_MESSAGE);
  CHECK(msg1.Body() == "{\"content\":\"embedded json body\"}");

  StringPtr str = Message::Serialize(msg1);
  Message msg2 = Message::Unserialize(str);
  CHECK(msg1 == msg2);
}

TEST(MessageUnittest, Serialize) {
  Message msg1;
  msg1.SetFrom("test");
  msg1.SetTo("user1");
  msg1.SetType(Message::T_MESSAGE);
  msg1.SetSeq(2);
  msg1.SetBody("this is a normal message body");

  StringPtr str = Message::Serialize(msg1);
  LOG(INFO) << *str;
  Message msg2 = Message::Unserialize(str);
  CHECK(msg1.HasSeq());
  CHECK(msg1.HasTo());
  CHECK(msg1.HasType());
  CHECK(msg1.To() == "user1");
  CHECK(msg1.From() == "test");
  CHECK(msg1.Seq() == 2);
  CHECK(msg1.Type() == Message::T_MESSAGE);
  CHECK(msg1.Body() == "this is a normal message body");
}

TEST(MessageUnittest, SerializeEmbed) {
  Message msg1;
  msg1.SetFrom("test");
  msg1.SetTo("user1");
  msg1.SetType(Message::T_MESSAGE);
  msg1.SetSeq(2);
  msg1.SetBody("{\"content\":\"this is a embedded message body\"}");

  StringPtr str = Message::Serialize(msg1);
  LOG(INFO) << *str;
  Message msg2 = Message::Unserialize(str);
  CHECK(msg1.HasSeq());
  CHECK(msg1.HasTo());
  CHECK(msg1.HasType());
  CHECK(msg1.To() == "user1");
  CHECK(msg1.From() == "test");
  CHECK(msg1.Seq() == 2);
  CHECK(msg1.Type() == Message::T_MESSAGE);
  CHECK(msg1.Body() == "{\"content\":\"this is a embedded message body\"}");
}

TEST(MessageUnittest, AckMessage) {
  Message ack;
  ack.SetSeq(100);
  ack.SetType(Message::T_ACK);
  CHECK(ack.HasType());
  CHECK(ack.Seq() == 100);
  CHECK(ack.Type() == Message::T_ACK);
  ack = Message::Unserialize(Message::Serialize(ack));
  CHECK(ack.HasType());
  CHECK(ack.Seq() == 100);
  CHECK(ack.Type() == Message::T_ACK);
}

TEST(MessageUnittest, SubMessage) {
  Message sub;
  sub.SetUser("user1");
  sub.SetChannel("channel1");
  sub.SetType(Message::T_SUBSCRIBE);
  CHECK(sub.HasType());
  CHECK(sub.User() == "user1");
  CHECK(sub.Channel() == "channel1");
  CHECK(sub.Type() == Message::T_SUBSCRIBE);
  sub = Message::Unserialize(Message::Serialize(sub));
  CHECK(sub.HasType());
  CHECK(sub.User() == "user1");
  CHECK(sub.Channel() == "channel1");
  CHECK(sub.Type() == Message::T_SUBSCRIBE);
}

TEST(MessageUnittest, UnubMessage) {
  Message unsub;
  unsub.SetUser("user1");
  unsub.SetChannel("channel1");
  unsub.SetType(Message::T_UNSUBSCRIBE);
  CHECK(unsub.HasType());
  CHECK(unsub.User() == "user1");
  CHECK(unsub.Channel() == "channel1");
  CHECK(unsub.Type() == Message::T_UNSUBSCRIBE);
  unsub = Message::Unserialize(Message::Serialize(unsub));
  CHECK(unsub.HasType());
  CHECK(unsub.User() == "user1");
  CHECK(unsub.Channel() == "channel1");
  CHECK(unsub.Type() == Message::T_UNSUBSCRIBE);
}

TEST(MessageUnittest, HeartBeatMessage) {
  Message hb;
  hb.SetType(Message::T_HEARTBEAT);
  CHECK(hb.HasType());
  CHECK(hb.Type() == Message::T_HEARTBEAT);
  hb = Message::Unserialize(Message::Serialize(hb));
  CHECK(hb.HasType());
  CHECK(hb.Type() == Message::T_HEARTBEAT);
}

TEST(MessageUnittest, ChannelMessage) {
  static const char* CMSG_BODY = "this is a channel message body";
  Message cmsg;
  cmsg.SetSeq(100);
  cmsg.SetType(Message::T_CHANNEL_MESSAGE);
  cmsg.SetFrom("test");
  cmsg.SetTo("user1");
  cmsg.SetChannel("channel1");
  cmsg.SetBody(CMSG_BODY);
  CHECK(cmsg.HasType());
  CHECK(cmsg.HasTo());
  CHECK(cmsg.HasSeq());
  CHECK(cmsg.Type() == Message::T_CHANNEL_MESSAGE);
  CHECK(cmsg.Seq() == 100);
  CHECK(cmsg.From() == "test");
  CHECK(cmsg.To() == "user1");
  CHECK(cmsg.Channel() == "channel1");
  CHECK(cmsg.Body() == CMSG_BODY);
  cmsg = Message::Unserialize(Message::Serialize(cmsg));
  CHECK(cmsg.HasType());
  CHECK(cmsg.HasTo());
  CHECK(cmsg.HasSeq());
  CHECK(cmsg.Type() == Message::T_CHANNEL_MESSAGE);
  CHECK(cmsg.Seq() == 100);
  CHECK(cmsg.From() == "test");
  CHECK(cmsg.To() == "user1");
  CHECK(cmsg.Channel() == "channel1");
  CHECK(cmsg.Body() == CMSG_BODY);
}

TEST(MessageUnittest, RealMessage) {
  static const char* MSG_BODY =
    "{\"type\":1040,\"ct\":\"湖北432\","
    "\"td\":\"55266c351267419c34f168aa\","
    "\"content\":\"4422\",\"url\":\"www.baidu.com\"}";
  Message msg1;
  msg1.SetType(Message::T_MESSAGE);
  msg1.SetTo("ff693d67a96fe86d4cf4fda8e4acf006");
  msg1.SetFrom("push_service");
  msg1.SetSeq(1);
  msg1.SetBody(MSG_BODY);
  
  Message msg2 = Message::Unserialize(Message::Serialize(msg1));
  CHECK(msg2 == msg1);
  CHECK(msg2.HasSeq());
  CHECK(msg2.HasTo());
  CHECK(msg2.HasSeq());
  CHECK(msg2.Seq() == 1);
  CHECK(msg2.Type() == Message::T_MESSAGE);
  CHECK(msg2.To() == "ff693d67a96fe86d4cf4fda8e4acf006");
  CHECK(msg2.From() == "push_service");
  CHECK(msg2.Body() == MSG_BODY);
}

}  // namespace xcomet
