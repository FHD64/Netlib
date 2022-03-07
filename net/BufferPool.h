#ifndef __NET_BUFFERPOOL__
#define __NET_BUFFERPOOL__

#include "noncopyable.h"
#include <atomic>
#include <vector>

namespace netlib {
namespace net {

//对齐BufferBlock
#if __SIZEOF_POINTER__ == 4
const uint16_t blockSize = 1024;
#elif __SIZEOF_POINTER__ == 8
const uint16_t blockSize = 1024+64-16;
#endif

struct BufferBlock {
    struct BufferBlock* next;
    uint16_t usage;
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