#include "CountDownLatch.h"
#include "Mutex.h"
#include "Thread.h"
#include "Timestamp.h"

#include <inttypes.h>
#include <vector>
#include <stdio.h>

using namespace netlib;
using namespace std;

Mutex g_mutex;
vector<int> g_vec;
const int kCount = 10 * 1000 * 1000;
int g_count = 0;

void threadFunc() {
  for (int i = 0; i < kCount; i++) {
    MutexLock lock(&g_mutex);
    g_vec.push_back(i);
  }
}

//初始调试时使用gcc默认优化级别O2，无法复现多线程下临界区访问而造成的数据错误问题。
//怀疑是gcc将 += 操作优化为原子操作，同理 ++操作编译器也会优化为原子操作。
//通过编译选项禁止优化下列两个线程函数，可以复现多线程下临界区访问造成的数据错误问题。
//实际使用中无须该编译选项，此处仅为调试所用。
#pragma GCC push_options
#pragma GCC optimize ("O0")
void threadFunc1() {
  for (int i = 0; i < kCount; i++) {
      MutexLock lock(&g_mutex);
      g_count += 1;
  }
}
void threadFunc2() {
    sleep(1);
    for (int i = 0; i < kCount; i++) {
        g_count += 1;
  }
}
#pragma GCC pop_options

void foo() {
    vector<unique_ptr<Thread>> Threads;
    const int threadNum = 16;
    g_count = 0;
    for(int i = 0; i < threadNum; i++) {
        Threads.emplace_back(new Thread(&threadFunc1));
    }
    for(int i = 0; i < threadNum; i++) {
        Threads[i]->start();
    }
    for(int i = 0; i < threadNum; i++) {
        Threads[i]->join();
    }
    printf("%d threads concurrent, g_count is % d, true number is %d \n", threadNum, g_count, threadNum * kCount);
}

inline int64_t timeDifference(Timestamp high, Timestamp low) {
  int64_t diff = high.getUsSinceEpoch() - low.getUsSinceEpoch();
  return diff;
}

void foo1() {
    vector<unique_ptr<Thread>> Threads;
    g_count = 0;
    const int threadNum = 16;
    for(int i = 0; i < threadNum; i++) {
        Threads.emplace_back(new Thread(&threadFunc2));
    }
    for(int i = 0; i < threadNum; i++) {
        Threads[i]->start();
    }
    for(int i = 0; i < threadNum; i++) {
        Threads[i]->join();
    }
    printf("%d threads concurrent with no lock, g_count is % d, true number is %d \n", threadNum, g_count, threadNum * kCount);
}

int main() {
    const int kMaxThreads = 8;
    foo();
    foo1();
    g_vec.reserve(kCount);
    Timestamp start(Timestamp::now());
    for (int i = 0; i < kCount; ++i) {
        g_vec.push_back(i);
    }

    printf("single thread without lock in MicroSeconds %" PRId64"\n", timeDifference(Timestamp::now(), start));

    start = Timestamp::now();
    threadFunc();
    printf("single thread with lock in MicroSeconds %" PRId64"\n", timeDifference(Timestamp::now(), start));

    for (int nthreads = 0; nthreads < kMaxThreads; nthreads++) {
        std::vector<std::unique_ptr<Thread>> threads;
        g_vec.clear();
        start = Timestamp::now();
        for (int i = 0; i <= nthreads; i++) {
            threads.emplace_back(new Thread(&threadFunc));
            threads.back()->start();
        }
        for (int i = 0; i <= nthreads; i++) {
            threads[i]->join();
        }
        printf("%d thread(s) with lock in MicroSeconds %" PRId64"\n", nthreads+1, timeDifference(Timestamp::now(), start));
    }
}
