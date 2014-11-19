
#ifndef BASE_PROTO_UTIL_H_
#define BASE_PROTO_UTIL_H_

#include <string>
#include "file/file.h"
#include "third_party/protobuf/include/google/protobuf/text_format.h"

namespace base {

namespace proto_util {

inline bool ParseFromASCII(const std::string &str,
                           google::protobuf::Message *message) {
  return google::protobuf::TextFormat::ParseFromString(str, message);
}

inline bool ConvertToString(const google::protobuf::Message &message,
                            std::string *str) {
  return google::protobuf::TextFormat::PrintToString(message, str);
}

inline bool ConvertToBinaryFile(const std::string &src_path,
                                const std::string &dst_path,
                                google::protobuf::Message *message) {
  if (message == NULL)
    return false;
  std::string content;
  if (!file::File::ReadFileToString(src_path, &content)) {
    return false;
  }
  message->Clear();
  if (!ParseFromASCII(content, message)) {
    return false;
  }
  std::string binary_format_str;
  message->SerializeToString(&binary_format_str);
  file::File::WriteStringToFile(binary_format_str, dst_path);
  return true;
}


}  // namespace proto_util

}  // namespace base

#endif  // BASE_PROTO_UTIL_H_
