#include "BufferPool.h"
#include "Buffer.h"
#include <cstdlib>

using namespace netlib;
using namespace netlib::net;

const uint16_t allocateSize = 10;
const uint16_t headSize = 8;
BufferPool::BufferPool() 
            : memUsage_(0),
              heads_(headSize, NULL) {
}

BufferPool::~BufferPool() {
    for (size_t i = 0; i < blocks_.size(); i++) {
        delete [] (blocks_[i]);
    }
}

BufferBlock* BufferPool::allocate(size_t size) {
    BufferBlock* block;
    if(size > (headSize * blockSize)) {
        block = reinterpret_cast<BufferBlock*>(new char[sizeof(BufferBlock) + size]);
        block->start = reinterpret_cast<char*>(block) + sizeof(BufferBlock);
        block->end = block->start + size;
        block->pos = block->start;
        block->last = block->start;
        block->next = NULL;
        return block;
    }
    size_t alloc = (size + blockSize - 1) & ~(blockSize - 1);
    size_t index = alloc / blockSize - 1;
    if(heads_[index]) {
        block = heads_[index];
        heads_[index] = heads_[index]->next;
        block->next = NULL;
        block->pos = block->start;
        block->last = block->start;
        memUsage_ -= alloc;
        return block;
    }

    fillPool(alloc);
    return allocate(size);
}

void BufferPool::fillPool(size_t size) {
    char* chunk = reinterpret_cast<char*>(new char[(sizeof(BufferBlock) + size) * allocateSize]);
    if(chunk == NULL) {
        return;
    }
    memUsage_ += (sizeof(BufferBlock) + size) * allocateSize;
    int i = 0;
    size_t index = size / blockSize - 1;
    while(1) {
        BufferBlock* current = reinterpret_cast<BufferBlock*>(chunk + i * (sizeof(BufferBlock) + size));
        current->start = reinterpret_cast<char*>(current) + sizeof(BufferBlock);
        current->end = current->start + size;
        current->pos = current->start;
        current->last = current->start;
        if(i == (allocateSize -1)) {
            current->next = heads_[index];
            break;
        }
        BufferBlock* next = reinterpret_cast<BufferBlock*>(chunk + (i + 1) * (sizeof(BufferBlock) + size));
        current->next = next;
        i++;
    }
    
    heads_[index] = reinterpret_cast<BufferBlock*>(chunk);
    blocks_.push_back(chunk);
}

void BufferPool::free(BufferBlock* block) {
    if(static_cast<size_t>(block->end - block->start) > (static_cast<size_t>(headSize) * blockSize)) {
        free(block);
        return;
    }
    size_t index = (block->end - block->start) / blockSize - 1;
    block->next = heads_[index];
    heads_[index] = block;
}