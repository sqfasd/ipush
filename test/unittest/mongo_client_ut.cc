#include "gtest/gtest.h"

#include "deps/base/logging.h"
#include "src/include_std.h"
#include "src/mongo_client.h"

namespace xcomet {

struct NameDevidPair {
  const char* devid;
  const char* name;
} user_infos[] = {
  {"fe41165a05b77f11bcebe79862c01590", "fe41165a05b77f11bcebe79862c01590"},
};

TEST(MongoClientUnittest, Normal) {
  MongoClient mongo;

  for (int i = 0; i < sizeof(user_infos) / sizeof(NameDevidPair); ++i) {
    StringPtr devid(new string(user_infos[i].devid));
    mongo.GetUserNameByDevId(devid, [i](Error err, StringPtr name) {
      CHECK(err == NO_ERROR) << err;
      CHECK(name.get());
      CHECK((*name) == user_infos[i].name) << *name;
    });
  }
}

}
