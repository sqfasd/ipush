#include "utils.h"

namespace xcomet {

void ParseIpPort(const string& address, string& ip, int& port) {
  size_t pos = address.find(':');
  if (pos == string::npos) {
    ip = "";
    port = 0;
    return;
  }
  ip = address.substr(0, pos);
  port = StringToInt(address.substr(pos+1, address.length()));
}

bool IsUserId(const string& id) {
  // TODO(qingfeng) recognize id type
  return id.find("c:") != 0;
}

} // namespace xcomet

