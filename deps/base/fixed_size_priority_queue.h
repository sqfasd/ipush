#ifndef BASE_FIXED_SIZE_PRIORITY_QUEUE_H_
#define BASE_FIXED_SIZE_PRIORITY_QUEUE_H_

#include <queue>
#include <vector>

#include "base/basictypes.h"

namespace base {

template<class Data, class Compare>
class FixedSizePriorityQueue {
 public:
  explicit FixedSizePriorityQueue(int max_size) {
    this->max_size = max_size;
  }

  ~FixedSizePriorityQueue() {}

  void Push(const Data& data) {
    if (queue.size() >= max_size) {
      if (!compare(queue.top(), data)) {
        queue.pop();
        queue.push(data);
      }
    } else {
      queue.push(data);
    }
  }

  bool Empty() const {
    return queue.empty();
  }

  Data const& Top() const {
    return queue.top();
  }

  void Pop() {
    queue.pop();
  }

  int Size() const {
    return queue.size();
  }

  void Clear() {
    while (!Empty()) Pop();
  }

  // Dump as priority ascending order
  void Flush(std::vector<Data>* array) {
    array->clear();
    while (!Empty()) {
      array->push_back(Top());
      Pop();
    }
  }

 protected:
  std::priority_queue<Data, std::vector<Data>, Compare> queue;
  int max_size;
  Compare compare;
};

}  // namespace base

#endif  // BASE_FIXED_SIZE_PRIORITY_QUEUE_H_
