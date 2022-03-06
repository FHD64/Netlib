#include "EventLoop.h"
#include "Thread.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

using namespace netlib;
using namespace netlib::net;

EventLoop g_loop;

void callback() {
    printf("callback(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
}

void threadFunc() {
    printf("threadFunc(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    g_loop.runInLoop(callback);
    g_loop.queueInLoop(callback);
    EventLoop loop;
    loop.runAfter(10.0, callback);
    loop.loop();
}

int main() {
    printf("main(): pid = %d, tid = %d\n", ::getpid(), CurrentThread::tid());

    Thread thread(threadFunc);
    thread.start();

    g_loop.loop();
}
