#ifndef __THREAD_CURRENTTHREAD__
#define __THREAD_CURRENTTHREAD__

#include <time.h>
#include <sys/syscall.h>
#include <syscall.h>
#include <unistd.h>
#include <cstdio>
#include <stdint.h>
#include "Timestamp.h"

namespace netlib
{

namespace CurrentThread {
 extern __thread int t_cachedTid;
 extern __thread char t_tidString[32];
 extern __thread int t_tidStringLength;
 extern __thread const char* t_threadName;
 //线程按进程划分，不同进程间的线程存在线程号相同的可能性
 //须通过系统调用获取线程真实线程号
 inline pid_t getTid() {
     return static_cast<pid_t>(::syscall(SYS_gettid));
 }
 inline int tid() {
     //避免反复查询进行系统调用，缓存当前线程tid
     if (t_cachedTid == 0) {
          t_cachedTid = getTid();
          t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%6d ", t_cachedTid);
     }
     return t_cachedTid;
 }
 inline const char* tidString() {
     return t_tidString;
 }
 inline int tidStringLength() {
     return t_tidStringLength;
 }
 inline const char* name() {
     return t_threadName;
 }
 inline void sleepUsec(int64_t usec) {
     struct timespec ts = {0, 0};
     ts.tv_sec = static_cast<time_t>(usec / netlib::Timestamp::kUsPerSecond);
     ts.tv_nsec = static_cast<long>(usec % (netlib::Timestamp::kUsPerSecond * 1000));
     ::nanosleep(&ts, NULL);
 }
}
}

#endif