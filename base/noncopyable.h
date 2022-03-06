#ifndef __BASE_NONCOPYABLE__
#define __BASE_NONCOPYABLE__

namespace netlib {

//通过声明拷贝构造及拷贝赋值为delete避免发生拷贝行为
class noncopyable {
 //C++11提供关键字 =delete
 public:
  noncopyable(const noncopyable&) = delete;
  void operator=(const noncopyable&) = delete;
 protected:
  noncopyable() = default;
  ~noncopyable() = default;
};

}

#endif