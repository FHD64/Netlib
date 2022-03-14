#ifndef __NET_BUFFER__
#define __NET_BUFFER__

#include "StringPiece.h"
#include "BufferPool.h"
#include "EventLoop.h"
#include <algorithm>
#include <vector>
#include <string.h>
#include <sys/uio.h>
#include <boost/operators.hpp>

namespace netlib {
namespace net {

class Buffer;

struct BufferIterator : public boost::equality_comparable<BufferIterator>,
                    public boost::less_than_comparable<BufferIterator> {
 size_t pos_;
 Buffer* buff_;
 BufferIterator() 
        : pos_(0),
          buff_(NULL) {
 }
 BufferIterator(size_t pos, Buffer* buff)
        : pos_(pos),
          buff_(buff) {
 }
 BufferIterator& operator++() {
     pos_++;
     return *this;
 }
 BufferIterator& operator--() {
     if(pos_ > 0) {
         pos_--;
     }
     return *this;
 }
 inline char& operator*();
};

inline BufferIterator operator-(BufferIterator it, int dis) {
    it.pos_ -= static_cast<size_t>(static_cast<int>(it.pos_) > dis ? static_cast<int>(it.pos_) - dis : 0);
    return it;
}

inline BufferIterator operator+(BufferIterator it, int dis) {
    it.pos_ += dis;
    return it;
}

inline size_t operator-(BufferIterator it1, BufferIterator it2) {
    if(it1.buff_ != it2.buff_) {
        return 0;
    }
    return it1.pos_ - it2.pos_;
}

inline bool operator<(BufferIterator it1, BufferIterator it2) {
    if(it1.buff_ == it2.buff_) {
        return it1.pos_ < it2.pos_;
    } 
    return false;
}

inline bool operator==(const BufferIterator it1, const BufferIterator it2) {
    if(it1.buff_ == it2.buff_ && it1.pos_ == it2.pos_) {
        return true;
    } else {
        return false;
    }
}

inline BufferIterator& operator+=(BufferIterator& it, int dis) {
    it = it + dis;
    return it;
}
inline BufferIterator& operator-=(BufferIterator& it, int dis) {
    it = it - dis;
    return it;
}
inline BufferIterator& operator+=(BufferIterator& it, size_t dis) {
    it = it + static_cast<int>(dis);
    return it;
}
inline BufferIterator& operator-=(BufferIterator& it, size_t dis) {
    it = it - static_cast<int>(dis);
    return it;
}
inline BufferIterator operator-(BufferIterator it, size_t dis) {
    it = it - static_cast<int>(dis);
    return it;
}
inline BufferIterator operator+(BufferIterator it, size_t dis) {
    it = it - static_cast<int>(dis);
    return it;
}

class Buffer {
 public:
  explicit Buffer(EventLoop* loop, size_t initsize = blockSize)
    : loop_(loop),
      size_(0),
      head_(NULL),
      writeIdx_(0),
      readIdx_(0) {
          int need = static_cast<int>((initsize + blockSize - 1) / blockSize);
          for(int i = 0; i < need; i++) {
              BufferBlock* block = loop_->allocate();
              if(block){
                  insert(block);
                  size_ += blockSize;
              }
          }
  }
  ~Buffer();
  BufferIterator begin() {
      return BufferIterator(readIdx_, this);
  }
  BufferIterator end() {
      return BufferIterator(writeIdx_, this);
  }
  size_t readableBytes() {
      return writeIdx_ - readIdx_;
  }
  size_t writeableBytes() {
      return size_ - writeIdx_;
  }
  BufferBlock* head() {
      return head_;
  }

  void retrieve() {
      readIdx_ = 0;
      writeIdx_ = 0;
  }
  EventLoop* loop() {
      return loop_;
  }
  void retrieveBytes(size_t len);
  void reset(EventLoop* loop);
  size_t copyToUser(char* dest, size_t destLen);
  //写入Buffer
  void append(const char* data, size_t len);
  void append(const StringPiece& str) {
      append(str.data(), str.size());
  }
  void append(const void* data, size_t len) {
      append(static_cast<const char*>(data), len);
  }
  void append(BufferIterator begin, BufferIterator end);
  
  void shrink(size_t reserve);
  //读取fd存至缓存区
  ssize_t readFd(int fd, int* savedErrno);
  ssize_t writeFd(int fd);
  friend struct BufferIterator;
 private:
  const static int maxIov = 20;

  void insert(BufferBlock* block) {
      if(!block) {
          return;
      }
      
      if(!head_) {
          block->next = block;
          block->prev = block;
          head_ = block;
      } else {
          head_->prev->next = block;
          block->prev = head_->prev;
          block->next = head_;
          head_->prev = block;
      }
  }

  void makeSpace(size_t len);
  size_t makeIov(int* iovsize, size_t begin, size_t end);
  EventLoop* loop_;
  size_t size_;
  BufferBlock* head_;
  size_t writeIdx_;
  size_t readIdx_;
  struct ::iovec iov_[maxIov];
};

inline char& BufferIterator::operator*() {
     size_t index = pos_ / blockSize;
     size_t offset = pos_ % blockSize;
     BufferBlock* block = buff_->head_;
     while(index > 0) {
         block = block->next;
         index--;
     }
     return block->buff[offset];
}
}
}
#endif