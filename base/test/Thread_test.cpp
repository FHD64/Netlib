#include "Thread.h"
#include "CurrentThread.h"

#include <string>
#include <stdio.h>
#include <unistd.h>

void mysleep(int seconds) {
  struct timespec t = { seconds, 0 };
  nanosleep(&t, NULL);
}

void threadFunc1() {
  printf("func1, thread name : %s, tid=%d\n", netlib::CurrentThread::t_threadName, netlib::CurrentThread::tid());
}

void threadFunc2(int x) {
  printf("func2, thread name : %s, tid=%d, x=%d\n", netlib::CurrentThread::t_threadName, netlib::CurrentThread::tid(), x);
}

void threadFunc3() {
  printf("func3, thread name : %s, tid=%d\n", netlib::CurrentThread::t_threadName, netlib::CurrentThread::tid());
  mysleep(1);
}

int main() {
  printf("init pthread pid=%d, tid=%d\n", ::getpid(), netlib::CurrentThread::tid());

  netlib::Thread t1(threadFunc1);
  t1.start();
  printf("t1.tid=%d\n", t1.tid());
  t1.join();

  netlib::Thread t2(std::bind(threadFunc2, 42));
  t2.start();
  printf("t2.tid=%d\n", t2.tid());
  t2.join();

  {
    netlib::Thread t3(threadFunc3);
    t3.start();
  }
  mysleep(2);
  {
    netlib::Thread t4(threadFunc3);
    t4.start();
    mysleep(2);
  }
  sleep(2);
  printf("number of created threads %d\n", netlib::Thread::numCreated());
}
