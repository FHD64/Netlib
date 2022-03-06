#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "Thread.h"

#include <stdio.h>
#include <unistd.h>

using namespace netlib;
using namespace netlib::net;

void printEventLoop(EventLoop* p = NULL) {
    printf("print: pid = %d, tid = %d, loop = %p\n", getpid(), CurrentThread::tid(), p);
}

int main() {
  printEventLoop();

  EventLoop loop;
  loop.runAfter(30, std::bind(&EventLoop::quit, &loop));

  {
      printf("Init thread %p:\n", &loop);
      EventLoopThreadPool model(&loop, "single");
      model.setThreadNum(0);
      model.start(printEventLoop);
  }

  {
      printf("Second thread:\n");
      EventLoopThreadPool model(&loop, "another");
      model.setThreadNum(1);
      model.start(printEventLoop);
      EventLoop* nextLoop = model.getLoop();
      nextLoop->runAfter(2, std::bind(printEventLoop, nextLoop));
      ::sleep(5);
  }

  {
      printf("Three threads:\n");
      EventLoopThreadPool model(&loop, "three");
      model.setThreadNum(3);
      model.start(printEventLoop);
      EventLoop* nextLoop = model.getLoop();
      nextLoop->runInLoop(std::bind(printEventLoop, nextLoop));
  }

  loop.loop();
}