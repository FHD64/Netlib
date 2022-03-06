#ifndef __NET_EVENTLOOPTHREAD__
#define __ENT_EVENTLOOPTHREAD__

#include "Thread.h"
#include "Mutex.h"
#include "Condition.h"
#include "noncopyable.h"

namespace netlib {

namespace net {

class EventLoop;
class EventLoopThread : public noncopyable {
 public:
  typedef std::function<void(EventLoop*)> ThreadInitCallback;
  EventLoopThread(ThreadInitCallback cb = ThreadInitCallback(), string name = string());
  ~EventLoopThread();
  EventLoop* startLoop();
 
 private:
  void threadFunc();
  EventLoop* loop_;
  bool exiting_;
  Thread thread_;
  Mutex mutex_;
  CondLock cond_;
  ThreadInitCallback callback_;
};
}
}
#endif