#ifndef SRC_MESSAGE_H_
#define SRC_MESSAGE_H_

#include <string>
#include "deps/jsoncpp/include/json/json.h"
#include "deps/base/logging.h"
#include "src/typedef.h"

namespace xcomet {

typedef base::shared_ptr<Json::Value> MessagePtr;
typedef base::shared_ptr<vector<string> > MessageResult;

class Message {
 public:
  static MessagePtr Unserialize(StringPtr data) {
    MessagePtr msg(new Json::Value());
    Json::Reader reader;
    if (!reader.parse(*data, *msg)) {
      LOG(ERROR) << "Unserialize failed";
    }
    return msg;
  }

  static string Serialize(MessagePtr msg) {
    if (msg.get() == NULL) {
      return "";
    }
    return Json::FastWriter().write(*msg);
  }

 private:
};

}  // namespace xcomet
#endif  // SRC_MESSAGE_H_
