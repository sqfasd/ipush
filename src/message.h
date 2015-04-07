#ifndef SRC_MESSAGE_H_
#define SRC_MESSAGE_H_

#include <string>
#include <ostream>
#include "deps/base/logging.h"
#include "src/typedef.h"
#include "src/utils.h"

namespace xcomet {

typedef shared_ptr<vector<string> > MessageDataSet;

const int64 NO_EXPIRE = 0;

#define K_FROM            "f"
#define K_TO              "t"
#define K_SEQ             "s"
#define K_TYPE            "y"
#define K_USER            "u"
#define K_CHANNEL         "c"
#define K_BODY            "b"

// only used for peer conmunication, removed before save or send to user
#define K_TTL             "l"

class Message {
 public:
  enum MType {
    T_HEARTBEAT,
    T_SUBSCRIBE,
    T_UNSUBSCRIBE,
    T_MESSAGE,
    T_CHANNEL_MESSAGE,
    T_ACK,
  };

  Message() : data_(new Json::Value()) {}
  ~Message() {}
  Message(const Message& other) {
    data_ = other.data_;
  }
  Message& operator=(const Message& other) {
    data_ = other.data_;
    return *this;
  }
  bool operator==(const Message& other) {
    return *data_ == *(other.data_);
  }
  void SetFrom(const string& uid) {
    (*data_)[K_FROM] = uid;
  }
  void SetTo(const string& uid) {
    (*data_)[K_TO] = uid;
  }
  void SetSeq(int seq) {
    (*data_)[K_SEQ] = seq;
  }
  void SetTTL(int64 ttl) {
    (*data_)[K_TTL] = (Json::Int64)ttl;
  }
  void SetType(MType type) {
    (*data_)[K_TYPE] = type;
  }
  void SetUser(const string& user) {
    (*data_)[K_USER] = user;
  }
  void SetChannel(const string& channel) {
    (*data_)[K_CHANNEL] = channel;
  }
  void SetBody(const string& body) {
    (*data_)[K_BODY] = body;
  }

  string From() const {
    return (*data_)[K_FROM].asString();
  }
  string To() const {
    return (*data_)[K_TO].asString();
  }
  int Seq() const {
    return (*data_)[K_SEQ].asInt();
  }
  int64 TTL() const {
    int64 ttl = 0;
    if (data_->isMember(K_TTL)) {
      ttl = (*data_)[K_TTL].asInt64();
      data_->removeMember(K_TTL);
    }
    return ttl;
  }
  MType Type() const {
    return (MType)(*data_)[K_TYPE].asInt();
  }
  string User() const {
    return (*data_)[K_USER].asString();
  }
  string Channel() const {
    return (*data_)[K_CHANNEL].asString();
  }
  string Body() {
    return (*data_)[K_BODY].asString();
  }

  bool HasSeq() const {
    return data_->isMember(K_SEQ);
  }
  bool HasType() const {
    return data_->isMember(K_TYPE);
  }
  bool HasTo() const {
    return data_->isMember(K_TO);
  }

  static Message Unserialize(StringPtr data) {
    // need to bind, cannot overload
    return UnserializeString(*data);
  }

  static Message UnserializeString(const string& data) {
    Message msg;
    Json::Reader reader;
    if (!reader.parse(data, *(msg.data_))) {
      LOG(ERROR) << "Unserialize failed: " << data;
    }
    return msg;
  }

  static StringPtr Serialize(Message msg) {
    StringPtr data(new string(""));
    SerializeJson(*(msg.data_), *data);
    return data;
  }

 private:
  shared_ptr<Json::Value> data_;

  friend ostream& operator<<(ostream&, const Message&);
};

inline ostream& operator<<(ostream& os, const Message& msg) {
  os << *(msg.data_);
  return os;
}

}  // namespace xcomet
#endif  // SRC_MESSAGE_H_
