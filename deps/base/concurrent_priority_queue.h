
#ifndef BASE_CONCURRENT_PRIORITY_QUEUE_H_
#define BASE_CONCURRENT_PRIORITY_QUEUE_H_

#include <queue>
#include <vector>

#include "base/basictypes.h"
#include "base/mutex.h"

namespace base {

template<class Data, class Compare>
class ConcurrentPriorityQueue {
 public:
  explicit ConcurrentPriorityQueue() {}

  ~ConcurrentPriorityQueue() {}

  void Push(const Data& data) {
    base::MutexLock lock(&mutex_);
    queue_.push(data);
    condition_variable_.Signal();
  }

  bool Empty() const {
    base::MutexLock lock(&mutex_);
    return queue_.empty();
  }

  Data const& Top() const {
    base::MutexLock lock(&mutex_);
    return queue_.top();
  }

  void Pop(Data* data) {
    base::MutexLock lock(&mutex_);
    while (queue_.empty()) {
      condition_variable_.Wait(&mutex_);
    }
    *data = queue_.top();
    queue_.pop();
  }

  int Size() const {
    base::MutexLock lock(&mutex_);
    return queue_.size();
  }

 protected:
  std::priority_queue<Data, std::vector<Data>, Compare> queue_;
  mutable base::Mutex mutex_;
  base::CondVar condition_variable_;
};

}  // namespace base

#endif  // BASE_CONCURRENT_PRIORITY_QUEUE_H_
