#ifndef SRC_SHARDING_H_
#define SRC_SHARDING_H_

#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include "deps/base/hash.h"
#include "deps/base/logging.h"

const int VIRTUAL_NODE_NUM = 100;

template<typename Node>
class Sharding {
 public:
  Sharding(const std::vector<Node>& nodes) {
    CHECK(nodes.size() > 0);
    for (int i = 0; i < nodes.size(); ++i) {
      for (int j = 0; j < VIRTUAL_NODE_NUM; ++j) {
        char buf[20] = {0};
        int n = ::sprintf(buf, "%d:%d", i, j);
        mapping_[base::Fingerprint(buf)] = nodes[i];
      }
    }
  }

  ~Sharding() {
  }

  const Node& operator[](const std::string& key) {
    uint64 hash = base::Fingerprint(key);
    typename Map::const_iterator iter = mapping_.lower_bound(hash);
    if (iter == mapping_.end()) {
      return mapping_.begin()->second;
    }
    return iter->second;
  }

 private:
  typedef std::map<uint64, Node> Map;
  Map mapping_;
};

#endif  // SHARDING_H_
