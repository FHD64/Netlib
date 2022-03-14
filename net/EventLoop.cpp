#include "EventLoop.h"
#include "Mutex.h"
#include "Logger.h"
#include "Channel.h"
#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <sys/epoll.h>

class IgnoreSigPipe {
 public:
  IgnoreSigPipe() {
      ::signal(SIGPIPE, SIG_IGN);
  }
};

static __thread netlib::net::EventLoop* t_loopInThisThread = 0;
static const int kEpollTimeMs = 10000;
static IgnoreSigPipe initsig;

static int createEventfd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0)
        LOG_SYSERR << "Failed in eventfd";
    
    return evtfd;
}

using namespace netlib;
using namespace netlib::net;

EventLoop* EventLoop::getEventLoopOfCurrentThread() {
    return t_loopInThisThread;
}

EventLoop::EventLoop()
    : looping_(false),
      callingpendingfunc_(false),
      threadid_(CurrentThread::tid()),
      epoller_(new Poller(this)),
      timerqueue_(new TimerQueue(this)),
      wakeupfd_(createEventfd()),
      wakeupchannel_(new Channel(this, wakeupfd_)),
      bufferPool_(new BufferPool()),
      connectionPool_(new TcpConnectionPool()) {
    if(t_loopInThisThread)
        LOG_FATAL << "Another loop is in this thread";
    else
        t_loopInThisThread = this;
    
    wakeupchannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    wakeupchannel_->enableReading();
}

EventLoop::~EventLoop() {
    wakeupchannel_->disableAll();
    wakeupchannel_->remove();
    ::close(wakeupfd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingpendingfunc_ = true;

    {
        //避免处理过程中加锁致使其他线程长时间等待
        MutexLock lock(&mutex_);
        functors.swap(pendingfunctors_);
    }

    for(const Functor& func : functors) {
        func();
    }
    
    callingpendingfunc_ = false;
}

void EventLoop::loop() {
    looping_ = true;

    while(looping_) {
        activechannels_.clear();
        epollreturntime_ = epoller_->epoll(kEpollTimeMs, &activechannels_);
        for(Channel* channel : activechannels_) {
            channel->handleEvent();
        }
        doPendingFunctors();
    }
}

void EventLoop::wakeup() {
    uint64_t i = 1;
    ssize_t n = ::write(wakeupfd_, &i, sizeof i);
    if(n != sizeof i) {
        LOG_ERROR << "EventLoop::wakeup()";
    }
}

void EventLoop::quit() {
    looping_ = false;
    if(!isInLoopThread()) {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb) {
    if(isInLoopThread()) {
        cb();
    } else {
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb) {
    {
        MutexLock lock(&mutex_);
        pendingfunctors_.push_back(cb);
    }

    if(!isInLoopThread() || callingpendingfunc_) {
        wakeup();
    }
} 

size_t EventLoop::pendQueueSize() const {
    MutexLock lock(&mutex_);
    return pendingfunctors_.size();
}

TimerId EventLoop::runAt(Timestamp time, TimeCallback cb) {
    return timerqueue_->addTimer(std::move(cb), time, 0.0);
}

TimerId EventLoop::runAfter(double delay, TimerCallback cb) {
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, std::move(cb));
}

TimerId EventLoop::runEvery(double interval, TimerCallback cb) {
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerqueue_->addTimer(std::move(cb), time, interval);
}

void EventLoop::cancelTimer(TimerId timerid) {
    return timerqueue_->cancel(timerid);
}

void EventLoop::updateChannel(Channel* channel) {
    epoller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel) {
    epoller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel) {
    return epoller_->hasChannel(channel);
}

void EventLoop::handleRead() {
    uint64_t i = 1;
    ssize_t n = ::read(wakeupfd_, &i, sizeof i);
    if(n != sizeof i) {
        LOG_ERROR << "EventLoop::handleRead()";
    }
}
