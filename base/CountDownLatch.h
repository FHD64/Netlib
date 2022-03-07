#ifndef __LOCK_COUNTERDOWNLATCH__
#define __LOCK_COUNTERDOWNLATCH__

#include "Condition.h"
#include "Mutex.h"

namespace netlib {

//计数器以互斥锁和条件锁共同实现
class CountDownLatch : noncopyable {
 public:
  explicit CountDownLatch(int count);
  void wait();
  void countDown();
  int getCount() const;
  void setCount(int count) {
      count_ = count;
  }
 
 private:
  //互斥锁
  mutable Mutex mutex_;
  CondLock cond_;
  int count_;
};

}
#endif