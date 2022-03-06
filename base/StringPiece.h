#ifndef __BASE_STRINGPIECE__
#define __BASE_STRINGPIECE__

#include <string>
#include <string.h>
#include <iosfwd>
#include <boost/operators.hpp>

namespace netlib
{

using std::string;

//修改StringPiece宏重载运算符写法，以boost::operator库实现
//StringPiece的出现是为了解决不必要的拷贝，String进行拷贝时会将对应字符重新拷贝，
//而许多场景下仅需拷贝指针即可
class StringPiece : public boost::equality_comparable<StringPiece>,
                    public boost::less_than_comparable<StringPiece> {
 public:
  StringPiece()
    : ptr_(NULL), length_(0) {
  }
  StringPiece(const char* str)
    : ptr_(str), length_(static_cast<int>(strlen(ptr_))) { 
  }
  StringPiece(const unsigned char* str)
    : ptr_(reinterpret_cast<const char*>(str)),
      length_(static_cast<int>(strlen(ptr_))) {
  }
  StringPiece(const string& str)
    : ptr_(str.data()), length_(static_cast<int>(str.size())) {
  }
  StringPiece(const char* offset, int len)
    : ptr_(offset), length_(len) {
  }

  const char* data() const {
      return ptr_;
  }
  int size() const {
      return length_; 
  }
  bool empty() const {
      return length_ == 0;
  }
  const char* begin() const { 
      return ptr_;
  }
  const char* end() const {
      return ptr_ + length_;
  }
  void clear() {
      ptr_ = NULL; length_ = 0;
  }
  void set(const char* buffer, int len) { 
      ptr_ = buffer;
      length_ = len;
  }
  void set(const char* str) {
      ptr_ = str;
      length_ = static_cast<int>(strlen(str));
  }
  void set(const void* buffer, int len) {
      ptr_ = reinterpret_cast<const char*>(buffer);
      length_ = len;
  }
  void removePrefix(int n) {
      ptr_ += n;
      length_ -= n;
  }
  void removeSuffix(int n) {
      length_ -= n;
  }

  //重载运算符[]
  char operator[](int i) const { 
      return ptr_[i];
  }
  //重载关系比较运算符
  bool operator==(const StringPiece& x) const {
      return ((length_ == x.length_) && (memcmp(ptr_, x.ptr_, length_) == 0));
  }      
  bool operator<(const StringPiece& x) const {                           
      int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_);
      return ((r < 0) || ((r == 0) && (length_ < x.length_)));
  }
  int compare(const StringPiece& x) const {
      int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_);
      if (r == 0) {
          if (length_ < x.length_)
              r = -1;
          else if (length_ > x.length_)
              r = 1;
      }
      return r;
  }

  string toString() const {
      return string(data(), size());
  }
  void copyToString(string* target) const {
      target->assign(ptr_, length_);
  }
  //判断x是否为当前stringpiece的前缀
  bool isPrefix(const StringPiece& x) const {
      return ((length_ >= x.length_) && (memcmp(ptr_, x.ptr_, x.length_) == 0));
  }

 private:
  const char*   ptr_;
  int           length_;
};
}

#endif