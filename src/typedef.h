#ifndef SRC_TYPEDEF_H_
#define SRC_TYPEDEF_H_

#include "src/include_std.h"

namespace xcomet {

typedef shared_ptr<std::string> StringPtr;
typedef const char* Error;

#define NO_ERROR ((const char*)NULL)

}

#endif  // SRC_TYPEDEF_H_
