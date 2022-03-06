#ifndef __THREAD_THREADPOOL__
#define __THREAD_THREADPOOL__

#include "Condition.h"
#include "Mutex.h"
#include "Thread.h"
#include <functional>
#include <deque>
#include <vector>

namespace netlib {

class ThreadPool : noncopyable {
 public:
  enum {
      SET_TASK_SUCCES,
      SET_TASK_FULL,
      SET_TASK_NOTHREAD
  };
  typedef std::function<void ()> Task;

  explicit ThreadPool(const string& nameArg = string("ThreadPool"));
  ThreadPool(size_t maxqsize, const string& nameArg = string("ThreadPool"));
  ~ThreadPool();

  void setMaxQueueSize(int maxSize) { 
      MutexLock lock(&mutex_);
      maxQueueSize_ = maxSize;
  }
  void setMaxQueueSizeWithLocked(int maxSize) { 
      maxQueueSize_ = maxSize;
  }
  void setThreadInitCallback(const Task& cb) { 
      MutexLock lock(&mutex_);
      threadInitCallback_ = cb;
  }
  void setThreadInitCallbackWithLocked(const Task& cb) { 
      threadInitCallback_ = cb;
  }

  void start(int numThreads);
  void stop();

  const string& name() const {
      return name_; 
  }

  size_t queueSize() const;
  //放置任务时可采取非阻塞方式
  int run(Task f, bool block = true);
  
 private:
  bool isFull() const;
  bool isFullWithLocked() const;
  void runInThread();
  Task take();

  mutable Mutex mutex_;
  CondLock notEmpty_;
  CondLock notFull_;
  string name_;
  Task threadInitCallback_;
  std::vector<std::unique_ptr<netlib::Thread>> threads_;
  std::deque<Task> queue_;
  size_t maxQueueSize_;
  bool running_;
};

}

#endif