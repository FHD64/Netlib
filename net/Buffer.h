#ifndef __NET_BUFFER__
#define __NET_BUFFER__

#include "StringPiece.h"
#include "BufferPool.h"
#include "EventLoop.h"
#include <algorithm>
#include <vector>
#include <string.h>
#include <sys/uio.h>

// TODO: 后续可优化为链式缓存

namespace netlib {

namespace net {

struct BufferIterator : public boost::equality_comparable<BufferIterator>,
                    public boost::less_than_comparable<BufferIterator> {
 BufferBlock* buff_;
 size_t pos_;
 BufferIterator()
       : buff_(NULL),
         pos_(0){
 }
 BufferIterator(BufferBlock* buff, size_t pos)
       : buff_(buff),
         pos_(pos){
 }
 BufferIterator& operator++() {
     if(pos_ == blockSize - 1) {
         if(buff_->next) {
             buff_ = buff_->next;
             pos_ = 0;
         } else {
             pos_ = blockSize;
         }
     } else {
         pos_++;
     }
     return *this;
 }
 BufferIterator& operator--() {
     if(pos_ == 0) {
         if(buff_->prev) {
             buff_ = buff_->prev;
             pos_ = blockSize - 1;
         }
     } else {
         pos_--;
     }
     return *this;
 }
 char& operator*() {
     return buff_->buff[pos_];
 }
};

inline bool operator==(const BufferIterator it1, const BufferIterator it2) {
    if(it1.buff_ == it2.buff_ && it1.pos_ == it2.pos_) {
        return true;
    } else {
        return false;
    }
}
BufferIterator operator-(BufferIterator it, int dis);
BufferIterator operator+(BufferIterator it, int dis);
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
inline BufferIterator& operator-=(BufferIterator& it, ssize_t dis) {
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

bool operator<(BufferIterator it1, BufferIterator it2);
size_t operator-(BufferIterator it1, BufferIterator it2);


class Buffer {
 public:
  explicit Buffer(EventLoop* loop, size_t initsize = blockSize)
    : loop_(loop),
      head_(loop_->allocate()),
      tail_(head_),
      readIt_(head_, 0),
      writeIt_(head_, 0) {
          head_->prev = NULL;
          int need = static_cast<int>((initsize + blockSize - 1) / blockSize);
          size_ = static_cast<size_t>(need * blockSize);
          for(int i = 0; i < need - 1; i++) {
              BufferBlock* block = loop_->allocate();
              insertTail(block);
          }
  }
  ~Buffer();
  BufferIterator begin() {
      return readIt_;
  }
  BufferIterator end() {
      return writeIt_;
  }
  size_t readableBytes() {
      return writeIt_ - readIt_;
  }
  size_t writeableBytes() {
      return BufferIterator(tail_, blockSize) - writeIt_;
  }

  void retrieve() {
      readIt_.buff_ = head_;
      readIt_.pos_ = 0;
      writeIt_.buff_ = head_;
      writeIt_.pos_ = 0;
  }
  void retrieveBytes(size_t len);

  size_t copyToUser(char* dest, size_t destLen);
  //写入Buffer
  void append(const char* data, size_t len);
  void append(const StringPiece& str) {
      append(str.data(), str.size());
  }
  void append(const void* data, size_t len) {
      append(static_cast<const char*>(data), len);
  }
  
  void shrink(size_t reserve);
  //读取fd存至缓存区
  ssize_t readFd(int fd, int* savedErrno);
  ssize_t writeFd(int fd);
  
 private:
  const static int maxIov = 10;

  BufferBlock* removeHead() {
      BufferBlock* temp = head_;
      if(head_ == tail_) {
          head_ = NULL;
          tail_ = NULL;
      } else {
          head_->next->prev = NULL;
          head_ = head_->next;
          temp->next = NULL;
          temp->prev = NULL;
      }
      return temp;
  }

  BufferBlock* removeTail() {
      BufferBlock* temp = tail_;
      if(head_ == tail_) {
          head_ = NULL;
          tail_ = NULL;
      }else {
          tail_->prev->next = NULL;
          tail_ = tail_->prev;
      }
      return temp;
  }

  void insertTail(BufferBlock* block) {
      if(!block) {
          return;
      }
      if(!tail_) {
          tail_ = block;
          head_ = block;
          tail_->prev = NULL;
          tail_->next = NULL;
      }else {
          block->next = NULL;
          block->prev = tail_;
          tail_->next = block;
          tail_ = block;
      }
  }

  void makeSpace(size_t len);
  size_t makeIov(int* iovsize, BufferIterator begin, BufferIterator end);
  EventLoop* loop_;
  BufferBlock* head_;
  BufferBlock* tail_;
  size_t size_;
  BufferIterator readIt_;
  BufferIterator writeIt_;
  struct ::iovec iov_[maxIov];
};

}
}


#endif