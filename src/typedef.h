#ifndef SRC_TYPEDEF_H_
#define SRC_TYPEDEF_H_

#include "src/include_std.h"

namespace xcomet {

typedef shared_ptr<std::string> StringPtr;
typedef shared_ptr<string> ErrorPtr;

#define NO_ERROR ErrorPtr(NULL)

}

#endif  // SRC_TYPEDEF_H_
