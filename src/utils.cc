#include "src/utils.h"

#include <fcntl.h>
#include <ctype.h>

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

void SetNonblock(int fd) {
  int flags;
  flags = ::fcntl(fd, F_GETFL);
  flags |= O_NONBLOCK;
  fcntl(fd, F_SETFL, flags);
  // TODO check ret != -1
}

} // namespace xcomet

