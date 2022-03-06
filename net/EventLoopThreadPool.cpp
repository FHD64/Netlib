#include "EventLoopThreadPool.h"

using namespace netlib;
using namespace netlib::net;

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseloop, const string& name)
    : baseloop_(baseloop),
      name_(name),
      started_(false), 
      numthreads_(0),
      next_(0){
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb) {
    started_ = true;

    for(int i = 0; i < numthreads_; i++) {
        char buf[name_.size() + 32];

        EventLoopThread* t = new EventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop());
    }

    if(numthreads_ == 0 && cb) {
        cb(baseloop_);
    }

}

EventLoop* EventLoopThreadPool::getLoop() {
    EventLoop* loop = baseloop_;
    if(!loops_.empty()) {
        loop = loops_[next_];
        next_ = (next_ + 1) % static_cast<int>(loops_.size());
    }

    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops() {
    if(loops_.empty()) {
        return std::vector<EventLoop*>(1, baseloop_);
    } else {
        return loops_;
    }
}