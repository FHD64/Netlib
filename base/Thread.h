#ifndef __THREAD_THREAD__
#define __THREAD_THREAD__

#include "noncopyable.h"
#include "CountDownLatch.h"
#include <functional>
#include <memory>
#include <pthread.h>
#include <atomic>

//linux中每个线程拥有的pid并非唯一，需通过系统调用获取线程tid
namespace netlib
{

//封装线程库
class Thread : noncopyable {
 public:
  typedef std::function<void ()> ThreadFunc;
  explicit Thread(ThreadFunc, const string& name = string());
  ~Thread();

  void start();
  int join();

  bool started() const { 
      return started_;
  }
  pid_t tid() const { 
      return tid_; 
  }
  const string& name() const { 
      return name_;
  }

  static int numCreated() { 
      return numCreated_;
  }

 private:
  void setDefaultName();
  bool       started_;
  bool       joined_;
  pthread_t  pid_;
  pid_t      tid_;
  ThreadFunc func_;
  string     name_;
  CountDownLatch latch_;
  static std::atomic<int> numCreated_;
};

}
#endif
