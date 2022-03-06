#ifndef __LOG_LOGGING__
#define __LOG_LOGGING__

#include "LogStream.h"
#include "Timestamp.h"

namespace netlib {

//日志采用C+++ stream写法，由以下几个部分封装而成
//1.Logstream 通过重载<<运算符，将打印内容存储在LogStream中的buffer内
//2.日志前缀附加时间戳, 行号, 文件名
//3.当前日志级别，通过与打印级别比较，可隐藏部分打印内容
class Logger {
 public:
  //记录日志等级
  enum LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    NUM_LOG_LEVELS,
  };

  //解析源文件名
  class SourceFile {
   public:
    template<int N>
    SourceFile(const char (&arr)[N])
      : data_(arr),
        size_(N-1) {
        const char* slash = strrchr(data_, '/'); // builtin function
        if (slash) {
            data_ = slash + 1;
            size_ -= static_cast<int>(data_ - arr);
        }
    }
    explicit SourceFile(const char* filename) : data_(filename) {
        const char* slash = strrchr(filename, '/');
        if (slash) {
          data_ = slash + 1;
        }
        size_ = static_cast<int>(strlen(data_));
    }
    const char* data_;
    int size_;
  };

  //每条打印语句至少需打印行号和源文件名
  Logger(SourceFile file, int line, LogLevel level);
  Logger(SourceFile file, int line, LogLevel level, const char* func);
  Logger(SourceFile file, int line, bool toAbort);
  ~Logger();
  
  LogStream& stream() { 
      return stream_; 
  }

  static LogLevel logLevel();
  static void setLogLevel(LogLevel level);
  
  typedef void (*OutputFunc)(const char* msg, int len);
  typedef void (*FlushFunc)();
  static void setOutput(OutputFunc);
  static void setFlush(FlushFunc);

 private:
  //每条日志前附加的日志信息，通过formatTime加入
  void formatTime();
  void finish();
  //日志时间戳
  Timestamp time_;
  //记录当前日志语句level_
  LogLevel level_;
  //日志行号
  int line_;
  SourceFile basename_;
  LogStream stream_;
};

extern Logger::LogLevel g_logLevel;
extern Logger::OutputFunc g_logOutput;
extern Logger::FlushFunc g_logFlush;

inline Logger::LogLevel Logger::logLevel() {
    return g_logLevel;
}

inline void Logger::setLogLevel(LogLevel level) {
    if(level < Logger::NUM_LOG_LEVELS) {
        g_logLevel = level;
        return;
    }
    g_logLevel = Logger::ERROR;
}

inline void Logger::setOutput(Logger::OutputFunc out) {
    if(out != NULL)
        g_logOutput = out;
}

inline void Logger::setFlush(Logger::FlushFunc flush) {
    if(flush != NULL)
        g_logFlush = flush;
}

#define LOG_TRACE \
    if (netlib::Logger::logLevel() <= netlib::Logger::TRACE) \
        netlib::Logger(__FILE__, __LINE__, netlib::Logger::TRACE, __func__).stream()
#define LOG_DEBUG \
    if (netlib::Logger::logLevel() <= netlib::Logger::DEBUG) \
        netlib::Logger(__FILE__, __LINE__, netlib::Logger::DEBUG, __func__).stream()
#define LOG_INFO \
    if (netlib::Logger::logLevel() <= netlib::Logger::INFO) \
        netlib::Logger(__FILE__, __LINE__, netlib::Logger::INFO).stream()
#define LOG_WARN netlib::Logger(__FILE__, __LINE__, netlib::Logger::WARN).stream()
#define LOG_ERROR netlib::Logger(__FILE__, __LINE__, netlib::Logger::ERROR).stream()
#define LOG_FATAL netlib::Logger(__FILE__, __LINE__, netlib::Logger::FATAL).stream()
#define LOG_SYSERR netlib::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL netlib::Logger(__FILE__, __LINE__, true).stream()

}

#endif