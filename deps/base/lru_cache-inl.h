#ifndef BASE_CACHE_LRU_CACHE_INL_H_
#define BASE_CACHE_LRU_CACHE_INL_H_

#include "base/lru_cache.h"

namespace base {

template<typename TypeKey, typename TypeValue>
LRUCache<TypeKey, TypeValue>::LRUCache(uint32 max_size) : max_size_(max_size) {}


template<typename TypeKey, typename TypeValue>
LRUCache<TypeKey, TypeValue>::~LRUCache() {
  Clear();
}

template<typename TypeKey, typename TypeValue>
base::shared_ptr<TypeValue> LRUCache<TypeKey, TypeValue>::Get(
    const TypeKey &key) {
  typename Map::iterator iter = index_.find(key);
  if (iter != index_.end()) {
    // invalidate iterator to spliced element
    value_list_.splice(value_list_.begin(), value_list_, iter->second);

    // update index.
    iter->second = value_list_.begin();

    return iter->second->second;
  }
  return base::shared_ptr<TypeValue>(NULL);
}

template<typename TypeKey, typename TypeValue>
void LRUCache<TypeKey, TypeValue>::Put(const TypeKey &key, TypeValue *value) {
  Put(key, base::shared_ptr<TypeValue>(value));
}

template<typename TypeKey, typename TypeValue>
void LRUCache<TypeKey, TypeValue>::Put(const TypeKey &key,
                                       base::shared_ptr<TypeValue> value) {
  {
    // remove if exists
    typename Map::iterator iter = index_.find(key);
    if (iter != index_.end()) {
      // The inserted value is the same with the one to be deleted.
      if (value.get() != NULL && iter->second->second.get() == value.get())
        return;
      RemoveValue(key);
    }
  }

  // insert to list
  value_list_.push_front(std::make_pair(key, value));

  // update index
  typename List::iterator iter = value_list_.begin();
  index_[key] = iter;

  // check overflow
  if (index_.size() > max_size_) {
    iter = value_list_.end();
    --iter;
    RemoveValue(iter->first);
  }
}

template<typename TypeKey, typename TypeValue>
void LRUCache<TypeKey, TypeValue>::Clear() {
  value_list_.clear();
  index_.clear();
}

}  // namespace base

#endif  // BASE_CACHE_LRU_CACHE_INL_H_
