#include "EventLoop.h"
#include "EventLoopThread.h"

using namespace netlib;
using namespace netlib::net;

EventLoopThread::EventLoopThread(ThreadInitCallback cb, string name) 
    : loop_(NULL),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this), name),
      mutex_(),
      cond_(&mutex_),
      callback_(cb){
}

EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if(loop_ != NULL) {
        loop_->quit();
        thread_.join();
    }
}

//由EventLoopThread生成的loop由用户管理
EventLoop* EventLoopThread::startLoop() {
    thread_.start();
    EventLoop* loop = NULL;
    {
        MutexLock lock(&mutex_);
        while(loop_ == NULL) {
            cond_.wait();
        }
        loop = loop_;
    }

    return loop;
}

void EventLoopThread::threadFunc() {
    EventLoop loop;

    if(callback_) {
        callback_(&loop);
    }
    
    {
        MutexLock lock(&mutex_);
        loop_ = &loop;
        cond_.notify();
    }
    loop_->loop();
    MutexLock lock(&mutex_);
    loop_ = NULL;
}