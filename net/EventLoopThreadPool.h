#ifndef __NET_EVENTLOOPTHREADPOOL__
#define __NET_EVENTLOOPTHREADPOOL__

#include "EventLoopThread.h"
#include "noncopyable.h"
#include <vector>
#include <memory>

namespace netlib {

namespace net {

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : public noncopyable {
 public:
  typedef std::function<void(EventLoop*)> ThreadInitCallback;

  EventLoopThreadPool(EventLoop* base, const string& name);
  ~EventLoopThreadPool() {
      //threads_中利用unique指针进行自动内存释放
  }
  
  void setThreadNum(int numthreads) {
      numthreads_ = numthreads;
  }
  void start(const ThreadInitCallback& cb = ThreadInitCallback());

  EventLoop* getLoop();
  std::vector<EventLoop*> getAllLoops();

  bool started() const {
      return started_;
  }
  const string& name() const {
      return name_;
  }

 private:
  EventLoop* baseloop_;
  string name_;
  bool started_;
  int numthreads_;
  int next_;
  std::vector<std::unique_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop*> loops_;
};
}
}
#endif