#include "Condition.h"
#include <errno.h>

//若超时返回true
bool netlib::CondLock::waitForMillisecond(int ms) {
    struct timespec abstime;
    clock_gettime(CLOCK_REALTIME, &abstime);

    int64_t ns = static_cast<int64_t>(ms * 1000000);

    abstime.tv_sec += static_cast<time_t>((abstime.tv_nsec + ns) / (1000 * 1000 * 1000));
    abstime.tv_nsec = static_cast<long>((abstime.tv_nsec + ns) % (1000 * 1000 * 1000));

    return (ETIMEDOUT == pthread_cond_timedwait(&pcond_, &(mutex_->mutex_), &abstime));
}