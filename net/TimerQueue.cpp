#include "TimerQueue.h"
#include "EventLoop.h"
#include <unistd.h>
#include <vector>

using namespace netlib;
using namespace netlib::net;

TimerQueue::TimerQueue(EventLoop* loop) 
    : loop_(loop),
      timerfd_(creatTimerfd()),
      timerfdChannel_(loop, timerfd_),
      timers_() {
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue() {
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);

    for (const Entry& timer : timers_)
        delete timer.second;
}

static struct timespec howMuchTimeFromNow(Timestamp when) {
    int64_t us = when.getUsSinceEpoch() - Timestamp::now().getUsSinceEpoch();
    us = us < 100 ? 100 : us;

    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(us / Timestamp::kUsPerSecond);
    ts.tv_nsec = static_cast<time_t>((us % Timestamp::kUsPerSecond) * 1000);

    return ts;
}

void TimerQueue::resetTimerfd(Timestamp expiration) {
    struct itimerspec newvalue;
    struct itimerspec oldvalue;

    memset(&newvalue, 0, sizeof newvalue);
    memset(&oldvalue, 0, sizeof oldvalue);
    newvalue.it_value = howMuchTimeFromNow(expiration);
    int ret = ::timerfd_settime(timerfd_, 0, &newvalue, &oldvalue);
    if(ret)
        LOG_SYSERR << "timerfd_settime()";
}

bool TimerQueue::insert(Timer* timer) {
    //判断当前timer是否需要刷新定时器响应时间
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if((it == timers_.end()) || (when < it->first))
        earliestChanged = true;
    
    timers_.insert(Entry(when, timer));
    return earliestChanged;
}

void TimerQueue::addTimerInLoop(Timer* timer) {
    bool earliestchanged = insert(timer);

    if(earliestchanged) {
        resetTimerfd(timer->expiration());
    }
}

TimerId TimerQueue::addTimer(TimerCallback cb, Timestamp when, double interval) {
    Timer* timer = new Timer(std::move(cb), when, interval);
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

void TimerQueue::handleRead() {
    Timestamp now(Timestamp::now());
    Timestamp nextexpire;

    readTimerfd(timerfd_);
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    TimerList::iterator begin = timers_.begin();
    TimerList::iterator end = timers_.lower_bound(sentry);
    std::vector<Entry> expired;
    
    //保存当前过期定时器
    std::copy(begin, end, std::back_inserter(expired));
    timers_.erase(begin, end);
    for (const Entry& it : expired) {
        it.second->run();
    }
    //重置定时器列表
    for(const Entry& it : expired) {
        if(it.second->repeat()) {
            it.second->restart(now);
            insert(it.second);
        } else {
            delete it.second;
        }
    }

    //重置定时器下次启动时间
    if(!timers_.empty()) {
        nextexpire = timers_.begin()->second->expiration();
    }
    if(nextexpire.valid()) {
        resetTimerfd(nextexpire);
    }
}

void TimerQueue::cancelInLoop(TimerId timerid) {
    size_t n = timers_.erase(Entry(timerid.timer_->expiration(), timerid.timer_));
    if(n != 1) {
        LOG_ERROR << "TimerQueue::cancelInLoop() " << n ;
        return;
    }

    delete timerid.timer_;
}

void TimerQueue::cancel(TimerId timerid) {
    loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerid));
}
