#ifndef XCOMET_SRC_UTILS_H
#define XCOMET_SRC_UTILS_H

#include <string>
#include "deps/jsoncpp/include/json/json.h"
#include "base/string_util.h"

namespace xcomet {

using namespace std;

void SetNonblock(int fd);

void ParseIpPort(const string& address, string& ip, int& port);

void SerializeJson(const Json::Value& json, string& data);

} // namespace xcomet

#endif
