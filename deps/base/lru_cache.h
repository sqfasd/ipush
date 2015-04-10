#ifndef BASE_CACHE_LRU_CACHE_H_
#define BASE_CACHE_LRU_CACHE_H_

#include <map>
#include <list>
#include <utility>

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/hash_tables.h"
#include "base/shared_ptr.h"

namespace base {

// This is a LRU cache.
// It is not thread safe.
template<typename TypeKey, typename TypeValue>
class LRUCache {
 public:
  explicit LRUCache(uint32 max_size);

  ~LRUCache();

  // Gets the value from the cache and also update the cache.
  // return NULL if no value found.
  base::shared_ptr<TypeValue> Get(const TypeKey &key);

  // Saves value in cache.
  // If the key already exists, the new value will replace the old one.
  // The cache will own the value and delete it when finish jobs.
  void Put(const TypeKey &key, TypeValue *value);

  // Saves value in cache.
  void Put(const TypeKey &key, base::shared_ptr<TypeValue> value);

  // Clear all values
  void Clear();

  int Size() const {
    return value_list_.size();
  }

 private:
  typedef std::list<std::pair<TypeKey, base::shared_ptr<TypeValue> > > List;
  typedef base::hash_map<TypeKey, typename List::iterator> Map;

  void RemoveValue(const TypeKey &key) {
    typename Map::iterator iter = index_.find(key);
    CHECK(iter != index_.end());

    value_list_.erase(iter->second);
    index_.erase(iter);
  }

  List value_list_;
  Map index_;
  uint32 max_size_;

  DISALLOW_COPY_AND_ASSIGN(LRUCache);
};

}  // namespace base

#endif  // BASE_CACHE_LRU_CACHE_H_
