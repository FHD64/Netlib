#include "ThreadPool.h"
#include <assert.h>
#include <stdio.h>

using namespace netlib;

ThreadPool::ThreadPool(const string& nameArg)
  : mutex_(),
    notEmpty_(&mutex_),
    notFull_(&mutex_),
    name_(nameArg),
    maxQueueSize_(0),
    running_(false) {
}

ThreadPool::ThreadPool(size_t maxqsize, const string& nameArg)
  : mutex_(),
    notEmpty_(&mutex_),
    notFull_(&mutex_),
    name_(nameArg),
    maxQueueSize_(maxqsize),
    running_(false) {
}

ThreadPool::~ThreadPool() {
    if (running_) {
        stop();
    }
}

void ThreadPool::start(int numThreads) {
  running_ = true;
  //扩充容量
  threads_.reserve(numThreads);
  for (int i = 0; i < numThreads; ++i) {
      char id[32];
      snprintf(id, sizeof id, "%d", i+1);
      threads_.emplace_back(new netlib::Thread(
            std::bind(&ThreadPool::runInThread, this), name_ + id));
      threads_[i]->start();
  }
}

void ThreadPool::stop() {
    {
        MutexLock lock(&mutex_);
        running_ = false;
        notEmpty_.notifyAll();
        notFull_.notifyAll();
    }
    for (auto& thr : threads_) {
        thr->join();
    }
}

size_t ThreadPool::queueSize() const {
    MutexLock lock(&mutex_);
    return queue_.size();
}

int ThreadPool::run(Task task, bool block) {
    MutexLock lock(&mutex_);
    if (threads_.empty()) {
        return SET_TASK_NOTHREAD;
    }
    if (block) {
        while (isFullWithLocked() && running_) 
            notFull_.wait();
    } else {
        if (isFullWithLocked() && running_)
            return SET_TASK_FULL;
    }
    if (!running_) 
        return SET_TASK_NOTHREAD;
    queue_.push_back(std::move(task));
    notEmpty_.notify();
    return SET_TASK_SUCCES;
}

ThreadPool::Task ThreadPool::take() {
    MutexLock lock(&mutex_);
    while (queue_.empty() && running_) {
        notEmpty_.wait();
    }
    Task task;
    if (!queue_.empty()) {
        task = queue_.front();
        queue_.pop_front();
        if (maxQueueSize_ > 0) {
            //通知阻塞于放置任务的线程
            notFull_.notify();
        }
    }
    return task;
}

bool ThreadPool::isFull() const {
    MutexLock lock(&mutex_);
    return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

bool ThreadPool::isFullWithLocked() const {
    return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

//线程池线程调用函数
void ThreadPool::runInThread() {
    if (threadInitCallback_) {
        threadInitCallback_();
    }

    while (running_) {
        Task task(take());
        if (task) {
            task();
        }
    }
}


