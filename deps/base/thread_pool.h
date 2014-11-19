
#ifndef BASE_THREAD_POOL_H_
#define BASE_THREAD_POOL_H_

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <queue>
#include <vector>

#include "base/basictypes.h"
#include "base/callback.h"
#include "base/dynamic_annotations.h"
#include "base/mutex.h"

namespace base {

class WorkerThread;

template <typename Type>
class ProducerConsumerQueue {
 public:
  // Capacity should be a positive number, INT_MAX means no capacity limit.
  // Negative and zero are invalid.
  explicit ProducerConsumerQueue(int capacity) {
    CHECK_GT(capacity, 0);
    capacity_ = capacity;
    ANNOTATE_PCQ_CREATE(this);
  }
  ~ProducerConsumerQueue() {
    CHECK(q_.empty());
    ANNOTATE_PCQ_DESTROY(this);
  }

  bool empty() {
    MutexLock lock(&mu_);
    return q_.empty();
  }

  // Put.
  void Put(Type* item) {
    MutexLock lock(&mu_);
    if (IsLimited()) {
      while (q_.size() >= capacity_) {
        not_full_.Wait(&mu_);
      }
    }
    CHECK(TryPutInternal(item));
  }

  // Returns true if succeed to add item to queue.
  // If queue is full, return false;
  bool TryPut(Type* item) {
    MutexLock lock(&mu_);
    return TryPutInternal(item);
  }

  // Get.
  // Blocks if the queue is empty.
  Type* Get() {
    MutexLock lock(&mu_);
    while (q_.empty())
      not_empty_.Wait(&mu_);
    Type* item = NULL;
    bool ok = TryGetInternal(&item);
    CHECK(ok);
    return item;
  }

  // If queue is not empty,
  // remove an element from queue, put it into *res and return true.
  // Otherwise return false.
  bool TryGet(Type **res) {
    MutexLock lock(&mu_);
    return TryGetInternal(res);
  }

  void Clear() {
    MutexLock lock(&mu_);
    Type* item = NULL;
    while (TryGetInternal(&item));
  }

  // Return the maximum number of elements.
  int capacity() { return capacity_; }

 private:
  // Returns true if the queue has a limited capacity.
  bool IsLimited() { return capacity_ != INT_MAX; }

  // Requires mu_
  bool TryGetInternal(Type** item_ptr) {
    if (q_.empty())
      return false;
    *item_ptr = q_.front();
    q_.pop();
    if (IsLimited()) {
      not_full_.Signal();
    }
    ANNOTATE_PCQ_GET(this);
    return true;
  }

  bool TryPutInternal(Type* item) {
    if (IsLimited() && (q_.size() >= capacity_)) {
      return false;
    }
    q_.push(item);
    not_empty_.Signal();
    ANNOTATE_CONDVAR_SIGNAL(&mu_);  // LockWhen in Get()
    ANNOTATE_PCQ_PUT(this);
    return true;
  }

  Mutex mu_;
  CondVar not_empty_;
  CondVar not_full_;
  int capacity_;
  std::queue<Type*> q_;  // protected by mu_
};

// A thread pool that uses ProducerConsumerQueue.
//   Usage:
//   {
//     ThreadPool pool(n_workers);
//     pool.StartWorkers();
//     pool.Add(NewOneTimeCallback(func_with_no_args));
//     pool.Add(NewOneTimeCallback(func_with_one_arg, arg));
//     pool.Add(NewOneTimeCallback(func_with_two_args, arg1, arg2));
//     ...  more calls to pool.Add()
//
//      the ~ThreadPool() is called: we wait workers to finish
//      and then join all threads in the pool.
//   }

class ThreadPool {
 public:
  // Create n_threads threads, but do not start.
  explicit ThreadPool(int n_threads);

  // Wait workers to finish, then join all threads.
  ~ThreadPool();

  // Start all threads.
  void StartWorkers();

  // Add a closure.
  void Add(Closure *closure) {
    queue_.Put(closure);
  }

  int num_threads() { return workers_.size();}
  int num_available_threads() { return num_available_workers_;}

 private:
  std::vector<WorkerThread*>  workers_;
  ProducerConsumerQueue<Closure> queue_;
  int num_available_workers_;
  Mutex lock_;

  static void* Worker(void *p);
};


/// Wrapper for pthread_create()/pthread_join().
class WorkerThread {
 public:
  typedef void *(*worker_t)(void* p);

  WorkerThread(worker_t worker,
               void *arg = NULL,
               const char *name = NULL)
      :w_(worker), arg_(arg), name_(name) {}

  WorkerThread(void (*worker)(void),
               void *arg = NULL,
               const char *name = NULL)
      :w_(reinterpret_cast<worker_t>(worker)), arg_(arg), name_(name) {}

  WorkerThread(void (*worker)(void* p),
               void *arg = NULL,
               const char *name = NULL)
      :w_(reinterpret_cast<worker_t>(worker)), arg_(arg), name_(name) {}

  ~WorkerThread() {
    w_ = NULL;
    arg_ = NULL;
  }

  void Start() {
    pthread_attr_t attr;
    CHECK_EQ(pthread_attr_init(&attr), 0);
    CHECK_EQ(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE),
             0);

    CHECK_EQ(pthread_create(&t_, &attr, (worker_t)ThreadBody, this), 0);
    CHECK_EQ(pthread_attr_destroy(&attr), 0);
  }

  void Join() {
    CHECK(0 == pthread_join(t_, NULL));
  }

  pthread_t tid() const { return t_; }

 private:
  static void ThreadBody(WorkerThread *my_thread) {
    if (my_thread->name_) {
      ANNOTATE_THREAD_NAME(my_thread->name_);
    }
    my_thread->w_(my_thread->arg_);
  }
  pthread_t t_;
  worker_t  w_;
  void     *arg_;
  const char *name_;
};

}  // namespace base

#endif  // BASE_THREAD_POOL_H_
