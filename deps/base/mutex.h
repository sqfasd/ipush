
#ifndef BASE_MUTEX_H_
#define BASE_MUTEX_H_

#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>  // mmap
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "base/basictypes.h"
#include "base/dynamic_annotations.h"
#include "base/logging.h"
#include "base/time.h"

namespace base {

class Mutex {
 public:
  // This is used for the single-arg constructor
  enum LinkerInitialized { LINKER_INITIALIZED };

  // Create a Mutex that is not held by anybody.  This constructor is
  // typically used for Mutexes allocated on the heap or the stack.
  inline Mutex();
  // This constructor should be used for global, static Mutex objects.
  // It inhibits work being done by the destructor, which makes it
  // safer for code that tries to acqiure this mutex in their global
  // destructor.
  inline Mutex(LinkerInitialized);

  // Destructor
  inline ~Mutex();

  inline void Lock();    // Block if needed until free then acquire exclusively
  inline void Unlock();  // Release a lock acquired via Lock()
  inline bool TryLock(); // If free, Lock() and return true, else return false

 private:
  friend class CondVar;
  pthread_mutex_t mu_;
  bool destroy_;
  DISALLOW_COPY_AND_ASSIGN(Mutex);
};
#define SAFE_PTHREAD(fncall) do {                          \
  if (fncall(&mu_) != 0) abort();                          \
} while (0)
Mutex::Mutex() : destroy_(true) {
  CHECK(0 == pthread_mutex_init(&mu_, NULL));
}
Mutex::Mutex(Mutex::LinkerInitialized) : destroy_(false) {
  CHECK(0 == pthread_mutex_init(&mu_, NULL));
}
Mutex::~Mutex() {
  if (destroy_)
    SAFE_PTHREAD(pthread_mutex_destroy);
}
void Mutex::Lock() {
  SAFE_PTHREAD(pthread_mutex_lock);
}
void Mutex::Unlock() {
  ANNOTATE_HAPPENS_BEFORE(this);
  SAFE_PTHREAD(pthread_mutex_unlock);
}
bool Mutex::TryLock()      { return pthread_mutex_trylock(&mu_) == 0; }
#undef SAFE_PTHREAD

// --------------------------------------------------------------------------
// Some helper classes

// MutexLock(mu) acquires mu when constructed and releases it when destroyed.
class MutexLock {
 public:
  explicit MutexLock(Mutex *mu) : mu_(mu) { mu_->Lock(); }
  ~MutexLock() { mu_->Unlock(); }
 private:
  Mutex * const mu_;
  // Disallow "evil" constructors
  MutexLock(const MutexLock&);
  void operator=(const MutexLock&);
};

// Read-Write mutex
class RwMutex {
 public:
  // This is used for the single-arg constructor
  enum LinkerInitialized { LINKER_INITIALIZED };

  // Create a Mutex that is not held by anybody.  This constructor is
  // typically used for Mutexes allocated on the heap or the stack.
  inline RwMutex();
  // This constructor should be used for global, static Mutex objects.
  // It inhibits work being done by the destructor, which makes it
  // safer for code that tries to acqiure this mutex in their global
  // destructor.
  inline RwMutex(LinkerInitialized);

  inline ~RwMutex();

  // Read / Write lock should not be used together with Lock / Unlock
  inline void ReaderLock();  // Block until free or shared then acquire a share
  inline void ReaderUnlock();  // Release a read share of this Mutex
  inline void WriterLock();  // Acquire an exclusive lock
  inline void WriterUnlock();  // Release a lock from WriterLock()
 private:
  inline void Unlock();
  pthread_rwlock_t mu_;
  bool destroy_;
  DISALLOW_COPY_AND_ASSIGN(RwMutex);
};
#define SAFE_PTHREAD(fncall) do {                          \
  if (fncall(&mu_) != 0) abort();                          \
} while (0)
RwMutex::RwMutex() : destroy_(true) {
  CHECK(0 == pthread_rwlock_init(&mu_, NULL));
}
RwMutex::RwMutex(RwMutex::LinkerInitialized) : destroy_(false) {
  CHECK(0 == pthread_rwlock_init(&mu_, NULL));
}
RwMutex::~RwMutex() {
  if (destroy_)
    SAFE_PTHREAD(pthread_rwlock_destroy);
}
void RwMutex::Unlock() {
  SAFE_PTHREAD(pthread_rwlock_unlock);
}
void RwMutex::ReaderLock() {
  SAFE_PTHREAD(pthread_rwlock_rdlock);
}
void RwMutex::ReaderUnlock() {
  Unlock();
}
void RwMutex::WriterLock() {
  SAFE_PTHREAD(pthread_rwlock_wrlock);
}
void RwMutex::WriterUnlock() {
  Unlock();
}
#undef SAFE_PTHREAD

// ReaderMutexLock and WriterMutexLock do the same, for rwlocks
class ReaderMutexLock {
 public:
  explicit ReaderMutexLock(RwMutex *mu) : mu_(mu) { mu_->ReaderLock(); }
  ~ReaderMutexLock() { mu_->ReaderUnlock(); }
 private:
  RwMutex * const mu_;
  DISALLOW_COPY_AND_ASSIGN(ReaderMutexLock);
};

class WriterMutexLock {
 public:
  explicit WriterMutexLock(RwMutex *mu) : mu_(mu) { mu_->WriterLock(); }
  ~WriterMutexLock() { mu_->WriterUnlock(); }
 private:
  RwMutex * const mu_;
  DISALLOW_COPY_AND_ASSIGN(WriterMutexLock);
};

// Catch bug where variable name is omitted, e.g. MutexLock (&mu);
#define MutexLock(x) COMPILE_ASSERT(0, mutex_lock_decl_missing_var_name)
#define ReaderMutexLock(x) COMPILE_ASSERT(0, rmutex_lock_decl_missing_var_name)
#define WriterMutexLock(x) COMPILE_ASSERT(0, wmutex_lock_decl_missing_var_name)

// A faster lock than mutex.
// SpinLock use less CPU instructions than Mutex, but may be less efficient if
// the lock area is huge and the race condition is frequent.
class SpinLock {
 public:
  SpinLock() {
    CHECK(0 == pthread_spin_init(&mu_, 0));
  }
  ~SpinLock() {
    CHECK(0 == pthread_spin_destroy(&mu_));
  }
  void Lock() {
    CHECK(0 == pthread_spin_lock(&mu_));
  }
  void Unlock() {
    CHECK(0 == pthread_spin_unlock(&mu_));
  }
 private:
  pthread_spinlock_t mu_;
};

/// Wrapper for pthread_cond_t.
class CondVar {
 public:
  CondVar()   { CHECK(0 == pthread_cond_init(&cv_, NULL)); }
  ~CondVar()  { CHECK(0 == pthread_cond_destroy(&cv_)); }
  void Wait(Mutex *mu) { CHECK(0 == pthread_cond_wait(&cv_, &mu->mu_)); }
  bool WaitWithTimeout(Mutex *mu, int millis) {
    struct timeval now;
    struct timespec timeout;
    gettimeofday(&now, NULL);
    timeval2timespec(&now, &timeout, millis);
    return 0 != pthread_cond_timedwait(&cv_, &mu->mu_, &timeout);
  }
  void Signal() { CHECK(0 == pthread_cond_signal(&cv_)); }
  void SignalAll() { CHECK(0 == pthread_cond_broadcast(&cv_)); }
 private:
  pthread_cond_t cv_;
};

class BlockingCounter {
 public:
  explicit BlockingCounter(int initial_count) :
    count_(initial_count) {}
  bool DecrementCount() {
    MutexLock lock(&mu_);
    count_--;
    if (count_ == 0)
      cond_.SignalAll();
    return count_ == 0;
  }
  void Wait() {
    MutexLock lock(&mu_);
    while(count_ != 0) {
      cond_.Wait(&mu_);
    }
  }
 private:
  Mutex mu_;
  CondVar cond_;
  int count_;
};

}  // namespace base

#endif  // #define BASE_MUTEX_H__
