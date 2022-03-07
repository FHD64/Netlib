#include "BufferPool.h"
#include <cstdlib>

using namespace netlib;
using namespace netlib::net;

const uint16_t allocateSize = 20;

BufferPool::BufferPool() 
            : memUsage_(0),
              head_(NULL) {
}

BufferPool::~BufferPool() {
    for (size_t i = 0; i < blocks_.size(); i++) {
        free(blocks_[i]);
    }
}

BufferBlock* BufferPool::allocate() {
    if(head_) {
        BufferBlock* temp = head_;
        head_ = head_->next;
        return temp;
    }

    fillPool();
    return allocate();
}

void BufferPool::fillPool() {
    BufferBlock* chunk = reinterpret_cast<BufferBlock*>(malloc(sizeof(BufferBlock) * allocateSize));
    if(chunk == NULL) {
        return;
    }

    int i = 0;
    while(1) {
        BufferBlock* current = chunk + i;
        BufferBlock* next = current + 1;
        if(i == (allocateSize -1)) {
            current->next = NULL;
            break;
        }
        current->next = next;
        i++;
    }

    head_ = chunk;
    blocks_.push_back(chunk);
}

void BufferPool::free(BufferBlock* block) {
    block->next = head_;
    head_ = block;
}