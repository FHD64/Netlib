#ifndef __NET_BUFFER__
#define __NET_BUFFER__

#include "StringPiece.h"
#include "BufferPool.h"
#include "EventLoop.h"
#include <algorithm>
#include <vector>
#include <string.h>
#include <sys/uio.h>
#include <cstring>
#include <boost/operators.hpp>

namespace netlib {
namespace net {

class Buffer;

const size_t blockSize = 4096;

struct BufferBlock {
    struct BufferBlock* next;
    char* start;
    char* end;
    char* pos;
    char* last;
};

class Buffer {
 public:
  explicit Buffer(EventLoop* loop, size_t initsize = blockSize)
    : loop_(loop),
      block_(NULL) {
      block_ = loop_->allocate(initsize);
  }
  ~Buffer();

  size_t readableBytes() {
      return block_->last - block_->pos;
  }
  size_t writeableBytes() {
      return block_->end - block_->last;
  }
  size_t size() {
      return block_->end - block_->start;
  }
  char* peek() {
      return block_->pos;
  }

  void retrieve() {
      block_->pos = block_->start;
      block_->last = block_->start; 
  }
  string retrieveAsString() {
      string result(peek(), readableBytes());
      retrieve();
      return result;
  }
  EventLoop* loop() {
      return loop_;
  }

  void retrieveBytes(size_t bytes) {
      if(bytes >= readableBytes()) {
          retrieve();
          return;
      }

      block_->pos += bytes;
  }

  void append(const char* data, size_t dataLen) {
      if(writeableBytes() < dataLen) {
          makeSpace(dataLen);
      }

      ::memcpy(block_->last, data, dataLen);
      block_->last += dataLen;
  }
  void append(const StringPiece& str) {
      append(str.data(), str.size());
  }
  void append(const void* data, size_t len) {
      append(static_cast<const char*>(data), len);
  }
  void release() {
      loop_->free(block_);
      block_ = NULL;
  }
  void reset(EventLoop* loop);
  void shrink(size_t reserve);
  //读取fd存至缓存区
  ssize_t readFd(int fd, int* savedErrno);
 private:
  void makeSpace(size_t len);
  EventLoop* loop_;
  BufferBlock* block_;
};

}
}
#endif