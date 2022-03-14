#ifndef __NET_BUFFERPOOL__
#define __NET_BUFFERPOOL__

#include "noncopyable.h"
#include <atomic>
#include <vector>

namespace netlib {
namespace net {

//对齐BufferBlock
const size_t blockSize = 4096;

struct BufferBlock {
    struct BufferBlock* next;
    struct BufferBlock* prev;
    char buff[blockSize];
};

class BufferPool : noncopyable {
 public:
  BufferPool();
  ~BufferPool();
  
  BufferBlock* allocate();
  void free(BufferBlock* buff);

  int memUsage() {
      return memUsage_.load();
  }
 private:
  void fillPool();
  std::atomic<int> memUsage_;
  BufferBlock* head_;
  std::vector<BufferBlock*> blocks_;
};
}
}
#endif