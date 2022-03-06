#include "Timestamp.h"
#include <sys/time.h>
#include <stdio.h>
#include <inttypes.h>

using namespace netlib;

string Timestamp::toString() const {
    char buf[32] = {0};
    int64_t s = ussinceepoch_ / kUsPerSecond;
    int64_t us = ussinceepoch_ % kUsPerSecond;
    snprintf(buf, sizeof(buf), "%010ld.%06ld", s, us);
    return buf;
}

string Timestamp::toFormattedString(bool showInus) const
{
  char buf[64] = {0};
  time_t seconds = static_cast<time_t>(ussinceepoch_ / kUsPerSecond);
  struct tm tm_time;
  //标准时间
  gmtime_r(&seconds, &tm_time);

  if (showInus) {
    int microseconds = static_cast<int>(ussinceepoch_ % kUsPerSecond);
    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
             microseconds);
  } else {
    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
  }
  return buf;
}

Timestamp Timestamp::now() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int64_t seconds = tv.tv_sec;
  return Timestamp(seconds * kUsPerSecond + tv.tv_usec);
}
