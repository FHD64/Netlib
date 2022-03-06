//条件锁使用原则
//1.与互斥锁配合使用，使用互斥锁保护条件判断检查
//2.使用while循环检查条件，避免spurious wakeup
#ifndef __LOCK_CONDITION__
#define __LOCK_CONDITION__

#include "Mutex.h"
#include "noncopyable.h"
#include <pthread.h>

namespace netlib {

class CondLock : noncopyable {
 public:
  explicit CondLock(Mutex* mutex) : mutex_(mutex) {
      pthread_cond_init(&pcond_, NULL);
  }
  ~CondLock() {
      pthread_cond_destroy(&pcond_);
  }
 
  void wait() {
      pthread_cond_wait(&pcond_, &mutex_->mutex_);
  }
  void notify() {
      pthread_cond_signal(&pcond_);
  }
  void notifyAll() {
      pthread_cond_broadcast(&pcond_);
  }

  bool waitForMillisecond(int ms);
 private:
  Mutex* const mutex_;
  pthread_cond_t pcond_;
};
}

#endif