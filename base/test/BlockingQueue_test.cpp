#include "BlockingQueue.h"
#include "CountDownLatch.h"
#include "Thread.h"

#include <memory>
#include <string>
#include <vector>
#include <stdio.h>
#include <unistd.h>

class Test {
 public:
  Test(int numThreads)
    : latch_(numThreads) {
    for (int i = 0; i < numThreads; ++i) {
      char name[32];
      snprintf(name, sizeof name, "work thread %d", i);
      threads_.emplace_back(new netlib::Thread(
            std::bind(&Test::threadFunc, this), std::string(name)));
    }
    for (auto& thr : threads_) {
      thr->start();
    }
  }

  void run(int times) {
    printf("waiting for count down latch\n");
    latch_.wait();
    printf("all threads started\n");
    for (int i = 0; i < times; ++i) {
      char buf[32];
      snprintf(buf, sizeof buf, "hello %d", i);
      queue_.put(buf);
      printf("tid=%d, put data = %s, size = %zd\n", netlib::CurrentThread::tid(), buf, queue_.size());
    }
  }

  void joinAll() {
    for (size_t i = 0; i < threads_.size(); ++i) {
      queue_.put("stop");
    }

    for (auto& thr : threads_) {
      thr->join();
    }
  }

 private:
  void threadFunc() {
      printf("tid=%d, %s started\n", netlib::CurrentThread::tid(), netlib::CurrentThread::name());

      latch_.countDown();
      bool running = true;
      while (running) {
          std::string d(queue_.take());
          printf("tid=%d, get data = %s, size = %zd\n", netlib::CurrentThread::tid(), d.c_str(), queue_.size());
          running = (d != "stop");
      }
      printf("tid=%d, %s stopped\n", netlib::CurrentThread::tid(), netlib::CurrentThread::name());
  }

  netlib::BlockingQueue<std::string> queue_;
  netlib::CountDownLatch latch_;
  std::vector<std::unique_ptr<netlib::Thread>> threads_;
};

int main() {
    printf("pid=%d, tid=%d\n", ::getpid(), netlib::CurrentThread::tid());
    Test t(5);
    t.run(100);
    t.joinAll();

    printf("number of created threads %d\n", netlib::Thread::numCreated());
}
