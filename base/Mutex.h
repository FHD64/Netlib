//互斥锁使用原则
//1.不使用跨进程mutex
//2.加锁、解锁保持在一个线程内
//3.不使用递归锁，避免代码逻辑错误
//  若线程A在加锁区域使用线程安全函数B，需生成函数B不加锁版本
#ifndef __LOCK_MUTEX__
#define __LOCK_MUTEX__

#include <pthread.h>
#include "noncopyable.h"
#include "CurrentThread.h"

//封装linux线程互斥锁
//GOOGLE代码风格：命名空间不增加额外缩进
namespace netlib {

//GOOGLE代码风格：public，protect，private关键词缩进一空格
//封装互斥锁
class Mutex : public noncopyable {
//GOOGLE代码风格：构造函数初始化列表放在同一行或按四格缩进并排多行
 public:
  Mutex() {
      pthread_mutex_init(&mutex_, NULL);
  }

  ~Mutex() {
      pthread_mutex_destroy(&mutex_);
  }

  void lock () {
      pthread_mutex_lock(&mutex_);
  }

  void unlock() {
      pthread_mutex_unlock(&mutex_);
  }
  
 private:
  friend class CondLock;
  pthread_mutex_t mutex_;
};

//RAII写法，资源获取即初始化
//互斥锁创建时上锁，销毁时解锁，根据变量作用域确定临界区
class MutexLock : noncopyable {
 public:
  explicit MutexLock(Mutex* mutex) : mutex_(mutex) {
      mutex_->lock();
  }
  ~MutexLock() {
      mutex_->unlock();
  }
 private:
  Mutex* const mutex_;
};
}

#endif