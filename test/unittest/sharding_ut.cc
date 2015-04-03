#include "gtest/gtest.h"

#include "deps/base/logging.h"
#include "src/include_std.h"
#include "src/sharding.h"

namespace xcomet {
class ShardingUnittest : public testing::Test {
 public:
  void SetNodeNum(int node_num) {
    vector<string> nodes;
    for (int i = 0; i < node_num; ++i) {
      char buf[20] = {0};
      sprintf(buf, "s%d", i);
      nodes.push_back(buf);
    }
    sharding_.reset(new Sharding<string>(nodes));
  }
  void CheckUserSharding(int user_num) {
    for (int i = 0; i < user_num ; ++i) {
      char buf[20] = {0};
      sprintf(buf, "user%d", i);
      LOG(INFO) << buf << "\t" << (*sharding_)[buf];
    }
  }
 private:
  shared_ptr<Sharding<string> > sharding_;
};

TEST_F(ShardingUnittest, SimpleTest) {
  // TODO(qingfeng) check the influenced ratio when nodes changed
  SetNodeNum(4);
  CheckUserSharding(10);

  SetNodeNum(5);
  CheckUserSharding(10);

  SetNodeNum(3);
  CheckUserSharding(10);
}

}
