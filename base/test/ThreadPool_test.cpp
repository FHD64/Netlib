#include "ThreadPool.h"
#include "Timestamp.h"

#include "CountDownLatch.h"
#include "CurrentThread.h"
#include "Logger.h"

#include <stdio.h>
#include <unistd.h>

#define testGroup(num) \
    do { \
        test(num, 0);\
        test(num, 1); \
        test(num, 5); \
        test(num, 10); \
        test(num, 50); \
    } while(0)

void printString(const std::string& str) {
    //LOG_INFO << str;
    usleep(100*1000);
}

void test(int numThread, int maxSize) {
    netlib::ThreadPool pool("MainThreadPool");
    pool.setMaxQueueSize(maxSize);
    pool.start(numThread);

    netlib::Timestamp start(netlib::Timestamp::now());
    LOG_WARN << "test1 Begin, max queue size :" << maxSize << " thread size : " << numThread;
    for (int i = 0; i < 100; ++i) {
      char buf[32];
      snprintf(buf, sizeof buf, "task %d", i);
      pool.run(std::bind(printString, std::string(buf)));
    }
    
    netlib::CountDownLatch latch(1);
    pool.run(std::bind(&netlib::CountDownLatch::countDown, &latch));
    latch.wait();
    pool.stop();
    LOG_WARN << "test1 Done, use time : " << netlib::Timestamp::now().getUsSinceEpoch() - start.getUsSinceEpoch();
}

int main() {
    testGroup(1);
    testGroup(3);
    testGroup(5);
    testGroup(7);
    testGroup(10);
}
