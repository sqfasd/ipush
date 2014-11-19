
#ifndef BASE_ENV_H_
#define BASE_ENV_H_

#include <string>
#include <utility>
#include <vector>

#include "base/flags.h"

DECLARE_bool(is_unit_test);

namespace base {

bool is_unit_test();

// get the building environment variable given one key.
bool GetBuildingEnv(const std::string &key, std::string *value);

// get all building environment variables.
void ListBuildingEnvs(std::vector<std::pair<std::string, std::string> > *vec);

// set the building environment variable.
// this method is not for normal code, do not call it in your method.
void SetBuildingEnv(const std::string &key, const std::string &value);

}  // namespace base

#endif  // BASE_ENV_H_
