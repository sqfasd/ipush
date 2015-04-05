#ifndef SRC_MESSAGE_H_
#define SRC_MESSAGE_H_

#include <string>
#include "deps/base/logging.h"
#include "src/typedef.h"
#include "src/utils.h"

namespace xcomet {

typedef shared_ptr<Json::Value> MessagePtr;
typedef shared_ptr<vector<string> > MessageDataSet;

// TODO(qingfeng) hide json as internal message implementation
class Message {
 public:
  static MessagePtr Unserialize(StringPtr data) {
    // need to bind, cannot overload
    return UnserializeString(*data);
  }

  static MessagePtr UnserializeString(const string& data) {
    MessagePtr msg(new Json::Value());
    Json::Reader reader;
    if (!reader.parse(data, *msg)) {
      LOG(ERROR) << "Unserialize failed: " << data;
    }
    return msg;
  }

  static StringPtr Serialize(MessagePtr msg) {
    StringPtr data(new string(""));
    if (msg.get()) {
      SerializeJson(*msg, *data);
    }
    return data;
  }

 private:
};

}  // namespace xcomet
#endif  // SRC_MESSAGE_H_
