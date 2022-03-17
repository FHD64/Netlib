#include "BufferPool.h"
#include "EventLoopThread.h"
#include "EventLoop.h"
#include "CountDownLatch.h"
#include <vector>
#include <atomic>
using namespace netlib;
using namespace netlib::net;

int allocateNum = 100000;
int threadNum = 5;
int size = 4096;
CountDownLatch count(threadNum);
CountDownLatch count1(threadNum);
std::atomic<long int> allocatetime(0);
std::atomic<long int> allocatefreetime(0);
std::atomic<long int> malloctime(0);
std::atomic<long int> freetime(0);
void allocateBuffer(EventLoop* loop) {
    std::vector<BufferBlock*> blocks;
    blocks.reserve(allocateNum);
    Timestamp start = Timestamp::now();
    printf("BufferPool start allocate : %s , tid : %d \n", start.toString().c_str(), CurrentThread::tid());
    
    for(int i = 0; i < allocateNum; i++) {
        blocks.push_back(loop->allocate(size));
    }
    printf("BufferPool end allocate : %s , tid : %d \n", Timestamp::now().toString().c_str(), CurrentThread::tid());
    allocatetime += Timestamp::now().getUsSinceEpoch()-start.getUsSinceEpoch();
    printf("BufferPool allocateNum : %d, allocate time %ld,tid : %d  \n", allocateNum, Timestamp::now().getUsSinceEpoch()-start.getUsSinceEpoch(), CurrentThread::tid());

    start = Timestamp::now();
    printf("BufferPool start free : %s , tid : %d \n", Timestamp::now().toString().c_str(), CurrentThread::tid());
    for(int i = 0; i < allocateNum; i++) {
        loop->free(blocks[i]);
    }
    allocatefreetime += Timestamp::now().getUsSinceEpoch()-start.getUsSinceEpoch();
    printf("BufferPool end free : %s , tid : %d \n", Timestamp::now().toString().c_str(), CurrentThread::tid());
    printf("BufferPool freeNum : %d, allocate time %ld,tid : %d\n", allocateNum, Timestamp::now().getUsSinceEpoch()-start.getUsSinceEpoch(), CurrentThread::tid());
    count.countDown();
}

void mallocBuffer(EventLoop* loop) {
    std::vector<BufferBlock*> blocks;
    blocks.reserve(allocateNum);
    Timestamp start = Timestamp::now();
    printf("malloc start allocate : %s , tid : %d \n", Timestamp::now().toString().c_str(), CurrentThread::tid());
    
    for(int i = 0; i < allocateNum; i++) {
        blocks.push_back(reinterpret_cast<BufferBlock*>(malloc(size * sizeof(char))));
    }
    printf("malloc end allocate : %s , tid : %d \n", Timestamp::now().toString().c_str(), CurrentThread::tid());
    malloctime += Timestamp::now().getUsSinceEpoch()-start.getUsSinceEpoch();
    printf("malloc allocateNum : %d, allocate time %ld,tid : %d  \n", allocateNum, Timestamp::now().getUsSinceEpoch()-start.getUsSinceEpoch(), CurrentThread::tid());

    start = Timestamp::now();
    printf("malloc start free : %s , tid : %d \n", Timestamp::now().toString().c_str(), CurrentThread::tid());
    for(int i = 0; i < allocateNum; i++) {
        free(blocks[i]);
    }
    freetime += Timestamp::now().getUsSinceEpoch()-start.getUsSinceEpoch();
    printf("malloc end free : %s , tid : %d \n", Timestamp::now().toString().c_str(), CurrentThread::tid());
    printf("malloc freeNum : %d, allocate time %ld,tid : %d  \n", allocateNum, Timestamp::now().getUsSinceEpoch()-start.getUsSinceEpoch(), CurrentThread::tid());
    count1.countDown();
}


int main(int argv, char* argc[]) {
    if(argv > 2) {
        allocateNum = atoi(argc[1]);
        threadNum = atoi(argc[2]);
        count.setCount(threadNum);
        count1.setCount(threadNum);
    }
    {
        std::vector<EventLoopThread*> threads;
        std::vector<EventLoop*> loops;
        for(int i = 0; i < threadNum; i++) {
            EventLoopThread* thr3 = new EventLoopThread();
            threads.push_back(thr3);
            loops.push_back(thr3->startLoop());
        }
        for(int i = 0; i < threadNum; i++) {
            loops[i]->runInLoop(std::bind(allocateBuffer, loops[i]));
        }
        CurrentThread::sleepUsec(5 * Timestamp::kUsPerSecond);
        count.wait();
        printf("allocatetime : %ld , allocatefreetime : %ld \n", allocatetime.load(), allocatefreetime.load());
    }

    {
        std::vector<EventLoopThread*> threads;
        std::vector<EventLoop*> loops;
        for(int i = 0; i < threadNum; i++) {
            EventLoopThread* thr3 = new EventLoopThread();
            threads.push_back(thr3);
            loops.push_back(thr3->startLoop());
        }
        for(int i = 0; i < threadNum; i++) {
            loops[i]->runInLoop(std::bind(mallocBuffer, loops[i]));
        }
        CurrentThread::sleepUsec(5 * Timestamp::kUsPerSecond);
        count1.wait();
        printf("malloctime : %ld , freetime : %ld \n", malloctime.load(), freetime.load());
    }
}