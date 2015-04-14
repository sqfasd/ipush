#ifndef BASE_THREADSAFE_LRU_CACHE_INL_H_
#define BASE_THREADSAFE_LRU_CACHE_INL_H_

#include "base/shared_ptr.h"
#include "base/threadsafe_lru_cache.h"
#include "base/lru_cache-inl.h"

namespace base {

template<typename TypeKey, typename TypeValue>
ThreadSafeLRUCache<TypeKey, TypeValue>::ThreadSafeLRUCache(uint32 max_size) {
  base::MutexLock lock(&mu_);
  cache_.reset(new LRUCache<TypeKey, TypeValue>(max_size));
}


template<typename TypeKey, typename TypeValue>
ThreadSafeLRUCache<TypeKey, TypeValue>::~ThreadSafeLRUCache() {
  base::MutexLock lock(&mu_);
  cache_->Clear();
}

template<typename TypeKey, typename TypeValue>
bool ThreadSafeLRUCache<TypeKey, TypeValue>::Get(
    const TypeKey &key, base::shared_ptr<TypeValue> *value) {
  base::MutexLock lock(&mu_);
  *value = cache_->Get(key);
  return value->get() != NULL;
}

template<typename TypeKey, typename TypeValue>
template<typename ReturnType>
ReturnType ThreadSafeLRUCache<TypeKey, TypeValue>::Call(
    const TypeKey &key,
    base::ResultCallback1<ReturnType, base::shared_ptr<TypeValue> > *callback,
    bool create_if_not_exist) {
  base::MutexLock lock(&mu_);
  base::shared_ptr<TypeValue> value;
  value = cache_->Get(key);
  if (value.get() == NULL && create_if_not_exist) {
    value.reset(new TypeValue);
    cache_->Put(key, value);
  }
  return callback->Run(value);
}

template<typename TypeKey, typename TypeValue>
void ThreadSafeLRUCache<TypeKey, TypeValue>::Call(
    const TypeKey &key,
    base::Callback1<base::shared_ptr<TypeValue> > *callback,
    bool create_if_not_exist) {
  base::MutexLock lock(&mu_);
  base::shared_ptr<TypeValue> value;
  value = cache_->Get(key);
  if (value.get() == NULL && create_if_not_exist) {
    value.reset(new TypeValue);
    cache_->Put(key, value);
  }
  callback->Run(value);
}

template<typename TypeKey, typename TypeValue>
void ThreadSafeLRUCache<TypeKey, TypeValue>::Put(const TypeKey &key,
                                                 TypeValue *value) {
  base::MutexLock lock(&mu_);
  cache_->Put(key, value);
}

template<typename TypeKey, typename TypeValue>
void ThreadSafeLRUCache<TypeKey, TypeValue>::Put(
    const TypeKey &key, base::shared_ptr<TypeValue> value) {
  base::MutexLock lock(&mu_);
  cache_->Put(key, value);
}

template<typename TypeKey, typename TypeValue>
void ThreadSafeLRUCache<TypeKey, TypeValue>::Clear() {
  base::MutexLock lock(&mu_);
  cache_->Clear();
}

}  // namespace base

#endif  // BASE_THREADSAFE_LRU_CACHE_INL_H_
