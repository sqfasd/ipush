#ifndef XCOMET_SRC_UTILS_H
#define XCOMET_SRC_UTILS_H

#include <string>
#include "base/string_util.h"

namespace xcomet {

using namespace std;

void ParseIpPort(const string& address, string& ip, int& port);

bool IsUserId(const string& id);

} // namespace xcomet

#endif
