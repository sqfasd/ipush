#ifndef SRC_BASE64_H_
#define SRC_BASE64_H_

#include "src/include_std.h"

namespace xcomet {

void Base64Encode(const char* input, int length, string& output);
void Base64Decode(const char* input, int length, string& output);

}  // namespace xcomet

#endif  // SRC_BASE64_H_
