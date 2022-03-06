#ifndef __NET_TIMERID__
#define __NET_TIMERID__

#include "Timer.h"

namespace netlib {

namespace net {

class TimerId {

 public:
  TimerId()
    : timer_(NULL),
      sequence_(0) {
  }
  
  TimerId(Timer* timer, int seq)
    : timer_(timer),
      sequence_(seq) {
  }

  friend class TimerQueue;

 private:
  Timer* timer_;
  int sequence_;
};
}
}

#endif