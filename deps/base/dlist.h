#ifndef BASE_DLIST_H_
#define BASE_DLIST_H_

#include <stddef.h>

namespace base {

template <class T>
class DLinkedList {
 public:
	class Iterator {
	 public:	
    Iterator(T cur) : cur_(cur) {
    }

		T Next() {
			T ret = cur_;
			if (cur_) {
				cur_ = cur_->next_;
			}
			return ret;
		}

	 private:
		T cur_;

	  friend class DLinkedList;
	};
	
	DLinkedList() : size_(0), head_(NULL), tail_(NULL) {
	}
	
	Iterator GetIterator() {
		return Iterator(head_);
	}
	
	bool Empty() const {
		return size_ == 0;
	}
	
	void Remove(T t) {
		this->size_--;
		if (t->prev_) {
			t->prev_->next_ = t->next_;
		}
		if (t->next_) {
			t->next_->prev_ = t->prev_;
		}
		if (this->head_ == t) {
			this->head_ = t->next_;
		}
		if (this->tail_ == t) {
			this->tail_ = t->prev_;
		}
	}
	
	T PopFront() {
		T t = this->head_;
		this->Remove(t);
		return t;
	}

	void PushBack(T t) {
		this->size_++;
		t->prev_ = this->tail_;
		t->next_ = NULL;
		if (this->tail_) {
			this->tail_->next_ = t;
		} else {
			this->head_ = t;
		}
		this->tail_ = t;
	}

 private:
	int size_;
	T head_;
	T tail_;
	friend class Iterator;
};
}  // namespace base

#endif  // BASE_DLIST_H_
