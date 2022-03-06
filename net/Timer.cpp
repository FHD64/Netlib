#include "Timer.h"

using namespace netlib;
using namespace netlib::net;

std::atomic<int> Timer::numcreated(0);

void Timer::restart(Timestamp now) {
    if(repeat_) {
        expiration_ = addTime(now, interval_);
    } else {
        expiration_ = Timestamp::invalid();
    }
}