#ifndef __NET_TIMER__
#define __NET_TIMER__

#include "Timestamp.h"
#include "noncopyable.h"
#include "Callback.h"
#include <atomic>

namespace netlib {

namespace net {

typedef std::function<void()> TimeCallback;
class Timer : noncopyable {
 public:
  Timer(TimeCallback cb, Timestamp when, double interval)
    : callback_(std::move(cb)),
      expiration_(when),
      interval_(interval),
      repeat_(interval > 0.0),
      sequence_(numcreated++) {
  }

  void run() const {
      callback_();
  }
  Timestamp expiration() const {
      return expiration_;
  }
  bool repeat() const {
      return repeat_;
  }
  int sequence() const {
      return sequence_;
  }
  double interval() const {
      return interval_;
  }
  void restart(Timestamp now);
  static int numCreated() {
      return numcreated;
  }
 private:
  const TimeCallback callback_;
  Timestamp expiration_;
  const double interval_;
  const bool repeat_;
  const int sequence_;
  static std::atomic<int> numcreated;
};
}
}

#endif
