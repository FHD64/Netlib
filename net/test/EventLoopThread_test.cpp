#include "EventLoopThread.h"
#include "EventLoop.h"
#include "Thread.h"
#include "CountDownLatch.h"

#include <stdio.h>
#include <unistd.h>

using namespace netlib;
using namespace netlib::net;

void printEventLoop(EventLoop* p = NULL) {
    printf("print: pid = %d, tid = %d, loop = %p\n", getpid(), CurrentThread::tid(), p);
}

void quit(EventLoop* p) {
    printEventLoop(p);
    p->quit();
}

int main() {
  printEventLoop();

    {
        EventLoopThread thr1;
    }

    {
        EventLoopThread thr2;
        EventLoop* loop = thr2.startLoop();
        loop->runInLoop(std::bind(printEventLoop, loop));
        CurrentThread::sleepUsec(5 * Timestamp::kUsPerSecond);
    }

    {
        EventLoopThread thr3;
        EventLoop* loop = thr3.startLoop();
        loop->runInLoop(std::bind(quit, loop));
        CurrentThread::sleepUsec(5 * Timestamp::kUsPerSecond);
    }
}