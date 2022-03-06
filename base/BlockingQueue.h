#ifndef __LOCK_BLOCKINGQUEUE__
#define __LOCK_BLOCKINGQUEUE__

#include "Condition.h"
#include "noncopyable.h"
#include "Mutex.h"
#include <deque>

namespace netlib {

template<typename T>
class BlockingQueue : noncopyable {
 public:
  BlockingQueue()
    : mutex_(),
      notEmpty_(&mutex_),
      queue_() {
  }

  void put(const T& x) {
      MutexLock lock(&mutex_);
      queue_.push_back(x);
      notEmpty_.notify();
  }

  void put(T&& x) {
      //针对临时对象，可通过右值引用提高效率
      MutexLock lock(&mutex_);
      queue_.push_back(std::move(x));
      notEmpty_.notify();
  }

  T take() {
      MutexLock lock(&mutex_);
      while (queue_.empty()) {
          notEmpty_.wait();
      }
      T front(std::move(queue_.front()));
      queue_.pop_front();
      return front;
  }

  std::deque<T> drain() {
      std::deque<T> queue;
      {
          MutexLock lock(&mutex_);
          queue = std::move(queue_);
      }
      return queue;
  }

  size_t size() const {
      MutexLock lock(&mutex_);
      return queue_.size();
  }

 private:
  mutable Mutex mutex_;
  CondLock notEmpty_;
  std::deque<T> queue_ ;
};

}
#endif
