#ifndef SRC_TYPEDEF_H_
#define SRC_TYPEDEF_H_

#include <string>
#include "deps/base/scoped_ptr.h"
#include "deps/base/shared_ptr.h"

namespace xcomet {

typedef std::string UserID;
typedef std::string ChannelID;
typedef int Sid;
typedef base::shared_ptr<std::string> StringPtr;

}

#endif  // SRC_TYPEDEF_H_
