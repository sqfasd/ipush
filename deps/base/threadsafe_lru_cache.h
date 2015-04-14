#ifndef BASE_THREADSAFE_LRU_CACHE_H_
#define BASE_THREADSAFE_LRU_CACHE_H_

#include <list>
#include <utility>

#include "base/basictypes.h"
#include "base/callback.h"
#include "base/mutex.h"
#include "base/scoped_ptr.h"
#include "base/shared_ptr.h"
#include "base/lru_cache.h"

namespace base {

// This is a thread safe LRU cache.
// It is actually a simple wrapper of LRUCache.
template<typename TypeKey, typename TypeValue>
class ThreadSafeLRUCache {
 public:
  explicit ThreadSafeLRUCache(uint32 max_size);

  ~ThreadSafeLRUCache();

  bool Get(const TypeKey &key, base::shared_ptr<TypeValue> *value);

  // Make sure thread safe for treatment of complicated logic.
  // If the key does not exists,
  // will create a new value through default constructor.
  template<typename ReturnType>
  ReturnType Call(const TypeKey &key,
      base::ResultCallback1<ReturnType, base::shared_ptr<TypeValue> > *callback,
      bool create_if_not_exist = false);
  void Call(const TypeKey &key,
      base::Callback1<base::shared_ptr<TypeValue> > *callback,
      bool create_if_not_exits = false);

  // Saves value in cache.
  // If the key already exists, the new value will replace the old one.
  // The cache will own the value and delete it when finish jobs.
  void Put(const TypeKey &key, TypeValue *value);
  void Put(const TypeKey &key, base::shared_ptr<TypeValue> value);

  uint32 Size() const {
    return cache_->Size();
  }

  // Clear all values
  void Clear();

 private:
  scoped_ptr<LRUCache<TypeKey, TypeValue> > cache_;
  base::Mutex mu_;

  DISALLOW_COPY_AND_ASSIGN(ThreadSafeLRUCache);
};

}  // namespace base

#endif  // BASE_THREADSAFE_LRU_CACHE_H_
