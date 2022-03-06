#ifndef __LOGSTREAM__
#define __LOGSTREAM__

#include "noncopyable.h"
#include "StringPiece.h"
#include <string.h>
namespace netlib
{

const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000 * 1000;

template<int SIZE>
class FixedBuffer : noncopyable {
 public:
  FixedBuffer() : cur_(data_) {
      bzero();
  }
  ~FixedBuffer() {
  }

  //在当前buffer后添加固定字节
  void append(const char* buf, size_t len) {
      if (static_cast<size_t>(avail()) > len) {
          memcpy(cur_, buf, len);
          cur_ += len;
      }
  }

  const char* data() const {
      return data_; 
  }
  int length() const { 
      return static_cast<int>(cur_ - data_); 
  }
  char* current() { 
      return cur_; 
  }
  void reset() { 
      cur_ = data_;
  }
  void bzero() { 
      memset(data_, 0, sizeof data_); 
  }
  void add(size_t len) {
      cur_ += len;
  }
  int avail() const { 
      return static_cast<int>(end() - cur_); 
  }
  //类型转换
  string toString() const { 
      return string(data_, length()); 
  }
  StringPiece toStringPiece() const { 
      return StringPiece(data_, length()); 
  }

 private:
  const char* end() const {
      return data_ + sizeof data_;
  }
  

  char data_[SIZE];
  char* cur_;
};

//LOG采用C++stream写法，需重载<<运算符
class LogStream : noncopyable {
 public:
  typedef FixedBuffer<kSmallBuffer> Buffer;

  LogStream& operator<<(short);
  LogStream& operator<<(unsigned short);
  LogStream& operator<<(int);
  LogStream& operator<<(unsigned int);
  LogStream& operator<<(long);
  LogStream& operator<<(unsigned long);
  LogStream& operator<<(long long);
  LogStream& operator<<(unsigned long long);
  LogStream& operator<<(const void*);
  LogStream& operator<<(double);

  LogStream& operator<<(float v) {
      *this << static_cast<double>(v);
      return *this;
  }
  LogStream& operator<<(bool v) {
      buffer_.append(v ? "1" : "0", 1);
      return *this;
  }
  LogStream& operator<<(char v) {
      buffer_.append(&v, 1);
      return *this;
  }
  LogStream& operator<<(const char* str) {
      if (str) {
          buffer_.append(str, strlen(str));
      } else {
          buffer_.append("(null)", 6);
      }
      return *this;
  }
  LogStream& operator<<(const unsigned char* str) {
      return operator<<(reinterpret_cast<const char*>(str));
  }
  LogStream& operator<<(const string& v) {
      buffer_.append(v.c_str(), v.size());
      return *this;
  }
  LogStream& operator<<(const StringPiece& v) {
      buffer_.append(v.data(), v.size());
      return *this;
  }
  LogStream& operator<<(const Buffer& v) {
      *this << v.toStringPiece();
      return *this;
  }

  void append(const char* data, int len) { 
      buffer_.append(data, len); 
  }
  const Buffer& buffer() const { 
      return buffer_;
  }
  void resetBuffer() { 
      buffer_.reset(); 
  }

 private:
  template<typename T> void formatInteger(T);
  Buffer buffer_;
  static const int kMaxNumericSize = 48;
};

class Fmt : noncopyable {
 public:
  template<typename T>
  Fmt(const char* fmt, T val);

  const char* data() const { 
      return buf_; 
  }
  int length() const { 
      return length_; 
  }

 private:
  char buf_[32];
  int length_;
};

inline LogStream& operator<<(LogStream& s, const Fmt& fmt) {
    s.append(fmt.data(), fmt.length());
    return s;
}

}

#endif
