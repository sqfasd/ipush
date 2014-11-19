// Wrapper gcc build-in atomic functions.
// In most cases, these builtins are considered a full barrier. For details,
// see:
// http://gcc.gnu.org/onlinedocs/gcc-4.4.3/gcc/Atomic-Builtins.html
// TODO(dahaili): replace and remove atomicops.h

#ifndef BASE_ATOMIC_H_
#define BASE_ATOMIC_H_

#include "base/basictypes.h"
#include "base/port.h"

namespace base {

// Atomically execute:
//      result = *ptr;
//      if (*ptr == old_value)
//        *ptr = new_value;
//      return result;
//
// I.e., replace "*ptr" with "new_value" if "*ptr" used to be "old_value".
// Always return the old value of "*ptr"
template<typename T>
T AtomicCompareAndSwap(volatile T* ptr,
                       T old_value,
                       T new_value) {
  return __sync_val_compare_and_swap(ptr, old_value, new_value);
}

// Atomically assign a new value to the pointer.
template<typename T>
void AtomicPointerAssgin(T* ptr, T new_value) {
  ignore_result(__sync_lock_test_and_set(ptr, new_value));
}

// Atomically increment *ptr by "increment".  Returns the new value of
// *ptr with the increment applied.  This routine implies no memory barriers.
template<typename T>
T AtomicIncrement(volatile T* ptr, T increment) {
  return __sync_add_and_fetch(ptr, increment);
}

// Atomically increment *ptr by 1.
template<typename T>
inline T AtomicIncrement(volatile T* ptr) {
  return AtomicIncrement(ptr, static_cast<T>(1));
}

template<typename T>
T AtomicDecrement(volatile T* ptr, T decrement) {
  return __sync_sub_and_fetch(ptr, decrement);
}

template<typename T>
inline T AtomicDecrement(volatile T* ptr) {
  return AtomicDecrement(ptr, static_cast<T>(1));
}

}  // namespace base

#endif  // BASE_ATOMIC_H_
