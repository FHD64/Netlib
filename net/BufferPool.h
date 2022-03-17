#ifndef __NET_BUFFERPOOL__
#define __NET_BUFFERPOOL__

#include "noncopyable.h"
#include <atomic>
#include <vector>

namespace netlib {
namespace net {

struct BufferBlock;
class BufferPool : noncopyable {
 public:
  BufferPool();
  ~BufferPool();
  
  BufferBlock* allocate(size_t size);
  void free(BufferBlock* buff);

  size_t memUsage() {
      return memUsage_.load();
  }
 private:
  void fillPool(size_t size);
  std::atomic<size_t> memUsage_;
  std::vector<BufferBlock*> heads_;
  std::vector<char*> blocks_;
};
}
}
#endif