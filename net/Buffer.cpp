#include "Buffer.h"
#include "Logger.h"
#include <errno.h>
#include <unistd.h>

using namespace netlib;
using namespace netlib::net;

void Buffer::makeSpace(size_t len) {
    if (writeableBytes() >= len){
        return;
    }
    size_t size = block_->end - block_->start;
    if((size - readableBytes()) >= len) {
        size_t bytes = readableBytes();
        memmove(block_->start, block_->pos, bytes);
        block_->pos = block_->start;
        block_->last = block_->pos + bytes;
        return;
    }
    
    BufferBlock* block = loop_->allocate(size + len);
    if(block) {
        memcpy(block->start, block_->pos, readableBytes());
        block->pos = block->start;
        block->last = block->pos + readableBytes();
        loop_->free(block_);
        block_ = block;
    }
}

Buffer::~Buffer() {
    loop_->free(block_);
}

ssize_t Buffer::readFd(int fd, int* savedErrno) {
    char extrabuf[65536];
    struct iovec vec[2];
    size_t len = writeableBytes();
    vec[0].iov_base = block_->last;
    vec[0].iov_len = len;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;
    ssize_t n = ::readv(fd, vec, 2);
    if(n < 0) {
        *savedErrno = errno;
    }else if(static_cast<size_t>(n) <= len) {
        block_->last += n;
    }else {
        block_->last += len;
        append(extrabuf, n - len);
    }

    return n;
}

void Buffer::shrink(size_t reserve) {
    size_t r = blockSize + readableBytes() > reserve ? readableBytes() + blockSize : reserve;
    r = (r + blockSize - 1) & ~(blockSize - 1);
    if(r >= static_cast<size_t>(block_->end - block_->start)) {
        return;
    }
    BufferBlock* block = loop_->allocate(r);
    memcpy(block->start, block_->pos, readableBytes());
    block->pos = block->start;
    block->last = block->pos + readableBytes();
    loop_->free(block_);
    block_ = block;
}

void Buffer::reset(EventLoop* loop) {
    if(block_) {
        if(loop == loop_) {
            block_->pos = block_->start;
            block_->last = block_->start;
            return;
        } else {
            loop_->free(block_);
        }
    }

    block_ = loop->allocate(blockSize);
    loop_ = loop;
}