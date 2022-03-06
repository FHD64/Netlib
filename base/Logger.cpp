#include "Logger.h"
#include "Timestamp.h"
#include "CurrentThread.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sstream>

namespace netlib {

__thread char t_errnobuf[512];
__thread char t_time[64];
__thread time_t t_lastSecond;
Logger::LogLevel g_logLevel = Logger::TRACE;

const char* LogLevelName[Logger::NUM_LOG_LEVELS] = {
  "[TRACE] ",
  "[DEBUG] ",
  "[INFO]  ",
  "[WARN]  ",
  "[ERROR] ",
  "[FATAL] ",
};

inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v) {
    s.append(v.data_, v.size_);
    return s;
}

void defaultOutput(const char* msg, int len) {
    fwrite(msg, 1, len, stderr);
}

void defaultFlush() {
    fflush(stderr);
}

Logger::OutputFunc g_logOutput = defaultOutput;
Logger::FlushFunc g_logFlush = defaultFlush;

}

using namespace netlib;

void Logger::formatTime() {
    int64_t ussinceepoch = time_.getUsSinceEpoch();
    time_t seconds = static_cast<time_t>(ussinceepoch / Timestamp::kUsPerSecond);
    int us = static_cast<int>(ussinceepoch % Timestamp::kUsPerSecond);
    //秒级日志提前缓存
    if (seconds != t_lastSecond) {
        t_lastSecond = seconds;
        struct tm tm_time;
        ::gmtime_r(&seconds, &tm_time);

        snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
                tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    }
    netlib::Fmt usfmt(".%06d ", us);
    stream_ << StringPiece(t_time, 17) << StringPiece(usfmt.data(), 8);
}

void Logger::finish() {
  stream_ << " - " << basename_ << ':' << line_ << '\n';
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
  : time_(Timestamp::now()),
    level_(level),
    line_(line),
    basename_(file),
    stream_() {
        formatTime();
        //避免第一次日志未记录线程号
        CurrentThread::tid();
        stream_ << StringPiece(CurrentThread::tidString(), CurrentThread::tidStringLength());
        stream_ << StringPiece(LogLevelName[level_], 8);
        stream_ << func << ' ';
}

Logger::Logger(SourceFile file, int line, LogLevel level)
  : time_(Timestamp::now()),
    level_(level),
    line_(line),
    basename_(file),
    stream_() {
        formatTime();
        //避免第一次日志未记录线程号
        CurrentThread::tid();
        stream_ << StringPiece(CurrentThread::tidString(), CurrentThread::tidStringLength());
        stream_ << StringPiece(LogLevelName[level_], 8);
}

Logger::Logger(SourceFile file, int line, bool toAbort)
  : time_(Timestamp::now()),
    level_(toAbort ? FATAL : ERROR),
    line_(line),
    basename_(file),
    stream_() {
        formatTime();
        //避免第一次日志未记录线程号
        CurrentThread::tid();
        stream_ << StringPiece(CurrentThread::tidString(), CurrentThread::tidStringLength());
        stream_ << StringPiece(LogLevelName[level_], 8);
        if(toAbort) {
            stream_ << strerror_r(errno, t_errnobuf, sizeof t_errnobuf);
            stream_ << " (errno=" << errno << ") ";
        }
}

Logger::~Logger() {
    finish();
    const LogStream::Buffer& buf(stream().buffer());
    g_logOutput(buf.data(), buf.length());
    if (level_ == FATAL) {
        g_logFlush();
    }
}