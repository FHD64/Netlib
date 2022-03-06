#include "Thread.h"
#include "CurrentThread.h"
#include "Logger.h"
#include <type_traits>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

namespace netlib {

std::atomic<int> Thread::numCreated_(0);

//TODO: 进程每次调用fork时，仅复制当前执行的线程，会将当前线程的锁状态fork至子进程
//若其他线程持有锁，会使fork进程出的子进程无法获取锁而死锁
//可通过pthread_atfork()进行锁获取及释放，避免子线程陷入死锁
//当前线程库不支持多线程fork
struct ThreadData {
    typedef netlib::Thread::ThreadFunc ThreadFunc;
    ThreadFunc func_;
    string name_;
    pid_t* tid_;
    CountDownLatch* latch_;

    ThreadData(ThreadFunc func,
               const string& name,
               pid_t* tid,
               CountDownLatch* latch)
      : func_(std::move(func)),
        name_(name),
        tid_(tid),
        latch_(latch) {
    }

    void runInThread() {
        //获取当前线程ID
        *tid_ = netlib::CurrentThread::tid();
        tid_ = NULL;
        latch_->countDown();
        latch_ = NULL;
        //刷新线程局部性变量
        netlib::CurrentThread::t_threadName = name_.empty() ? "netlibThread" : name_.c_str();
        //设置线程名
        ::prctl(PR_SET_NAME, netlib::CurrentThread::t_threadName);

        func_();
        netlib::CurrentThread::t_threadName = "finished";
    }
};

//使用startThread函数及传入void* obj必要性
//1.每个线程有自己的线程局部性变量保存相关线程信息，需要在线程创建完成更新，
//因此不可以在start函数中更新，因为此时还停留在主线程(创建线程)，无法更新子
//线程的线程局部性变量
//2.父线程调用创建线程后，随时可能释放Thread对象，而此刻Thread保存的func_变量无法确定其值的正确性，
//需要保存一份副本传入startThread以使得正常调用
void* startThread(void* obj) {
    ThreadData* data = static_cast<ThreadData*>(obj);
    data->runInThread();
    delete data;
    return NULL;
}

Thread::Thread(ThreadFunc func, const string& n)
  : started_(false),
    joined_(false),
    pid_(0),
    tid_(0),
    func_(std::move(func)),
    name_(n),
    latch_(1) {
        setDefaultName();
}

Thread::~Thread() {
    if (started_ && !joined_) {
        pthread_detach(pid_);
    }
}

void Thread::setDefaultName() {
    int num = numCreated_++;
    if (name_.empty()) {
        char buf[32];
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}

void Thread::start() {
    started_ = true;
    ThreadData* data = new ThreadData(func_, name_, &tid_, &latch_);
    if (pthread_create(&pid_, NULL, &startThread, data)) {
        started_ = false;
        delete data;
        LOG_SYSFATAL << "Failed in pthread_create";
    }
    else {
        //等待创建的线程成功运行
        latch_.wait();
    }
}

int Thread::join() {
    joined_ = true;
    return pthread_join(pid_, NULL);
}
}