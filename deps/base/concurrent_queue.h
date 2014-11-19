
#ifndef BASE_CONCURRENT_QUEUE_H_
#define BASE_CONCURRENT_QUEUE_H_

#include <queue>

#include "base/basictypes.h"
#include "base/mutex.h"

namespace base {

template<class Data>
class ConcurrentQueue {
 public:
  ConcurrentQueue() {}
  virtual ~ConcurrentQueue() {}

  virtual void Push(const Data& data) {
    base::MutexLock lock(&mutex_);
    queue_.push(data);
    condition_variable_.Signal();
  }

  virtual void Pop(Data& data) {
    base::MutexLock lock(&mutex_);

    while (queue_.empty()) {
      condition_variable_.Wait(&mutex_);
    }
    data = queue_.front();
    queue_.pop();
  }

  bool TryPop(Data& data) {
    base::MutexLock lock(&mutex_);
    if (queue_.empty()) {
      return false;
    }
    data = queue_.front();
    queue_.pop();
    return true;
  }

  Data const& Front() const {
    base::MutexLock lock(&mutex_);
    return queue_.front();
  }

  bool Empty() const {
    base::MutexLock lock(&mutex_);
    return queue_.empty();
  }

  int Size() const {
    base::MutexLock lock(&mutex_);
    return queue_.size();
  }

 protected:
  std::queue<Data> queue_;
  mutable base::Mutex mutex_;
  base::CondVar condition_variable_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ConcurrentQueue);
};

template<typename Data>
class FixedSizeConQueue : public ConcurrentQueue<Data> {
 public:
  explicit FixedSizeConQueue(int max_count = 256) : max_count_(max_count) {}
  virtual ~FixedSizeConQueue() {}

  virtual void Push(const Data& data) {
    base::MutexLock lock(&this->mutex_);
    while (this->queue_.size() >= max_count_) {
      condition_full_.Wait(&(this->mutex_));
    }
    this->queue_.push(data);
    this->condition_variable_.Signal();
  }

  virtual void Pop(Data& data) {
    base::MutexLock lock(&this->mutex_);

    while (this->queue_.empty()) {
      this->condition_variable_.Wait(&this->mutex_);
    }
    data = this->queue_.front();
    this->queue_.pop();
    condition_full_.Signal();
  }

  bool Full() const {
    return this->queue_.size() >= max_count_;
  }

 private:
  base::CondVar condition_full_;
  const int max_count_;

  DISALLOW_COPY_AND_ASSIGN(FixedSizeConQueue);
};


}  // namespace base

#endif  // BASE_CONCURRENT_QUEUE_H_
