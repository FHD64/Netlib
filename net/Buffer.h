#ifndef __NET_BUFFER__
#define __NET_BUFFER__

#include "StringPiece.h"
#include <algorithm>
#include <vector>
#include <string.h>

// TODO: 后续可优化为链式缓存

namespace netlib {

namespace net {

//当前缓存由vectoc<char>组成，总共将缓存分为三部分
//1.前缀:在Buffer前留出冗余空间，以增添数据头
//2.可读部分：通过readidx指示可读部分的第一个字节，读出数据后readidx++
//3.可写部分：通过writeidx指示可写部分的第一个字节，写入数据后wirteidx++
//
//****************缓存空间示意******************/
//+-前缀-+--------可读部分--------+---可写部分----+
//    readidx                 writeidx
//
//****************缓存存放机制******************/
//1.通过移动数据释放可用空间
//2.通过vector.resize申请vector空间
//3.提供接口直接解析Buffer，避免用户通过二次复制浪费性能
//4.read接口为系统调用，为了减少系统调用次数且与内存使用之间取得平衡，

class Buffer {
 public:
  //前方预留10字节空间
  static const int kprepend = 10;
  static const int kinitsize = 1024;
  explicit Buffer(size_t initsize = kinitsize)
    : buffer_(kprepend + kinitsize),
      readidx_(kprepend),
      writeidx_(kprepend) {
  }

  //当前Buffer内存实现通过vector实现，而非动态分配内存，可以考虑不实现
  //拷贝构造、移动构造、拷贝赋值、移动赋值函数而仅实现swap函数
  void swap(Buffer& rhs) {
      buffer_.swap(rhs.buffer_);
      std::swap(readidx_, rhs.readidx_);
      std::swap(writeidx_, rhs.writeidx_);
  }
  
  size_t readableBytes() const {
      return writeidx_ - readidx_;
  }
  size_t writeableBytes() const {
      return buffer_.size() - writeidx_;
  }
  size_t prependBytes() const {
      return readidx_;
  }

  const char* peek() const {
      return begin() + readidx_;
  }

  //清空Buffer
  void retrieve() {
      readidx_ = kprepend;
      writeidx_ = kprepend;
  }
  void retrieveBytes(size_t len) {
      if(len > readableBytes())
          retrieve();
      readidx_ += len;
  }
  string retrieveAsString() {
      string result(peek(), readableBytes());
      retrieve();
      return result;
  }
  string retrieveBytesAsString(size_t len) {
      if(len > readableBytes())
          return retrieveAsString();

      string result(peek(), len);
      retrieveBytes(len);
      return result;
  }
  StringPiece retrieveAsStringPiece() {
      return StringPiece(peek(), static_cast<int>(readableBytes()));
  }
  
  //写入Buffer
  void append(const char* data, size_t len) {
      if(writeableBytes() < len)
          makeSpace(len);
      std::copy(data, data + len, begin() + writeidx_);
      writeidx_ += len;
  }
  void append(const StringPiece& str) {
      append(str.data(), str.size());
  }
  void append(const void* data, size_t len) {
      append(static_cast<const char*>(data), len);
  }

  //设置头前缀
  int setPrepend(const void* data, size_t len) {
      if(len > prependBytes())
          return -1;

      readidx_ -= len;
      const char* d = static_cast<const char*>(data);
      std::copy(d, d + len, begin() + readidx_);
      return 0;
  }
  
  void shrink(size_t reserve) {
      size_t r = readableBytes() + reserve + kprepend;
      Buffer newbuf(r > kinitsize ? r : kinitsize);
      newbuf.append(retrieveAsStringPiece());
      swap(newbuf);
  }
  //读取fd存至缓存区
  ssize_t readFd(int fd, int* savedErrno);

 private:
  const char* begin() const {
      return &(*buffer_.begin());
  }
  char* begin() {
      return &(*buffer_.begin());
  }
  void makeSpace(size_t len);
  std::vector<char> buffer_;
  size_t readidx_;
  size_t writeidx_;
};

}
}

#endif