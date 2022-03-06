#include "CountDownLatch.h"

using namespace netlib;

CountDownLatch::CountDownLatch(int count)
    : mutex_(), cond_(&mutex_), count_(count) {

}

void CountDownLatch::wait() {
    MutexLock lock(&mutex_);

    while(count_ > 0) {
        cond_.wait();
    }
}

void CountDownLatch::countDown() {
    MutexLock lock(&mutex_);

    count_--;
    if(count_ == 0)
        cond_.notifyAll();
}

int CountDownLatch::getCount() const {
    MutexLock lock(&mutex_);
    return count_;
}