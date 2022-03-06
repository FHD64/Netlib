#ifndef __NET_TIMERQUEUE__
#define __NET_TIMERQUEUE__

#include "Channel.h"
#include "Timer.h"
#include "TimerId.h"
#include "noncopyable.h"
#include "Logger.h"
#include "Callback.h"
#include <sys/timerfd.h>
#include <set>

namespace netlib {

namespace net {

class EventLoop;
class TimerQueue : noncopyable {
 public:
  explicit TimerQueue(EventLoop* loop);
  ~TimerQueue();

  TimerId addTimer(TimerCallback cb, Timestamp when, double interval);
  void cancel(TimerId timerid);

 private:
  typedef std::pair<Timestamp, Timer*> Entry;
  typedef std::set<Entry> TimerList;

  int creatTimerfd() {
      return ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  }
  void resetTimerfd(Timestamp expiration);
  void readTimerfd(int timerfd) {
      uint64_t howmany;
      ssize_t n = ::read(timerfd_, &howmany, sizeof howmany);
      if (n != sizeof howmany) {
          LOG_ERROR << "TimerQueue::readTimefd() reads " << n << "bytes instead of 8";
      }
  }
  void addTimerInLoop(Timer* timer);
  void cancelInLoop(TimerId timerid);
  void handleRead();
  bool insert(Timer* timer);

  EventLoop* loop_;
  const int timerfd_;
  Channel timerfdChannel_;
  TimerList timers_;
};
}
}
#endif