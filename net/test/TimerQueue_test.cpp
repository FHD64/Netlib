#include "EventLoop.h"
#include "EventLoopThread.h"
#include "Thread.h"

#include <stdio.h>
#include <unistd.h>

using namespace netlib;
using namespace netlib::net;

int cnt = 0;
EventLoop* g_loop;

void printTid() {
    printf("pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    printf("now %s\n", Timestamp::now().toString().c_str());
}

void printMessage(const char* msg) {
    printf("msg %s %s\n", Timestamp::now().toString().c_str(), msg);
    if (++cnt == 20) {
        g_loop->quit();
    }
}

void cancel(TimerId timer, const char *name) {
    g_loop->cancelTimer(timer);
    printf("cancelled at %s %s\n", Timestamp::now().toString().c_str(), name);
}

const char* t45name = "t45";
const char* t3name = "t3";

int main() {
    printTid();
    sleep(1);
    {
        EventLoop loop;
        g_loop = &loop;

        printMessage("main");
        loop.runAfter(1, std::bind(printMessage, "once1"));
        loop.runAfter(1.5, std::bind(printMessage, "once1.5"));
        loop.runAfter(2.5, std::bind(printMessage, "once2.5"));
        loop.runAfter(3.5, std::bind(printMessage, "once3.5"));
        TimerId t45 = loop.runAfter(4.5, std::bind(printMessage, "once4.5"));
        loop.runAfter(4.2, std::bind(cancel, t45, t45name));
        loop.runAfter(4.8, std::bind(cancel, t45, t45name));
        loop.runEvery(2, std::bind(printMessage, "every2"));
        //TimerId t3 = 
        loop.runEvery(3, std::bind(printMessage, "every3"));
        //loop.runAfter(9.001, std::bind(cancel, t3, t3name));

        loop.loop();
        printMessage("main loop exits");
    }
    sleep(1);
    {
        EventLoopThread loopThread;
        EventLoop* loop = loopThread.startLoop();
        loop->runAfter(2, printTid);
        sleep(3);
        printMessage("thread loop exits \n");
    }
}