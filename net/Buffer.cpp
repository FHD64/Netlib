#include "Buffer.h"
#include "Logger.h"
#include <errno.h>
#include <unistd.h>
#include <cstring>

using namespace netlib;
using namespace netlib::net;

void Buffer::makeSpace(size_t len) {
    if (writeableBytes() >= len){
        return;
    }
    
    size_t expansion = (len - writeableBytes() + blockSize - 1) & (~(blockSize - 1));
    if(size_ < 16 * 1024) {
        expansion = expansion > size_ ? expansion : size_;
    }

    while (expansion > 0) {
        BufferBlock* block = loop_->allocate();
        if(block) {
            insert(loop_->allocate());
            size_ += blockSize;
        }
        expansion -= blockSize;
    }
}

Buffer::~Buffer() {
    while(head_->next != head_) {
        BufferBlock* block = head_->next;
        head_->next = block->next;
        loop_->free(block);
    }
    loop_->free(head_);
}

size_t Buffer::makeIov(int* iovsize, size_t begin, size_t end) {
    *iovsize = 0;

    size_t size = 0;
    size_t beginIndex = begin / blockSize;
    BufferBlock* beginBlock  = head_;
    while(beginIndex) {
        beginBlock = beginBlock->next;
        beginIndex--;
    }

    while(*iovsize < maxIov && begin < end) {
        if((end - begin) <= (blockSize - (begin % blockSize))) {
            iov_[*iovsize].iov_base = beginBlock->buff + (begin % blockSize);
            iov_[*iovsize].iov_len = end - begin;
            size += end - begin;
            (*iovsize)++;
            break;
        }else {
            iov_[*iovsize].iov_base = beginBlock->buff + (begin % blockSize);
            iov_[*iovsize].iov_len = blockSize - (begin % blockSize);
            begin += blockSize - (begin % blockSize);
            beginBlock = beginBlock->next;
            size += blockSize - (begin % blockSize);
            (*iovsize)++;
        } 
    }

    return size;
}

ssize_t Buffer::readFd(int fd, int* savedErrno) {
    char extrabuf[65536];
    int iovsize = 0;
    size_t len = makeIov(&iovsize, writeIdx_, size_);
    
    if(iovsize < maxIov) {
        iov_[iovsize].iov_base = extrabuf;
        iov_[iovsize].iov_len = sizeof extrabuf;
        iovsize++;
    }

    ssize_t n = ::readv(fd, iov_, iovsize);
    if(n < 0) {
        *savedErrno = errno;
    }else if(static_cast<size_t>(n) <= len) {
        writeIdx_ += n;
    }else {
        writeIdx_ += len;
        append(extrabuf, n - len);
    }

    return n;
}

ssize_t Buffer::writeFd(int fd) {
    int iovsize = 0;
    size_t len = makeIov(&iovsize, readIdx_, writeIdx_);
    len = len;
    ssize_t n = ::writev(fd, iov_, iovsize);
    return n;
}

size_t Buffer::copyToUser(char* dest, size_t destLen) {
    size_t len = 0;
    size_t begin = readIdx_;
    size_t beginIdx = readIdx_ / blockSize;
    BufferBlock* block = head_;
    while(beginIdx) {
        block = block->next;
        beginIdx--;
    }
    while((begin < writeIdx_) && (len < destLen)) {
        size_t endpos = (writeIdx_ - begin) <= ((blockSize - (readIdx_ % blockSize))) ? 
                                    (writeIdx_ - begin) : ((blockSize - (readIdx_ % blockSize)));
        char* end = block->buff + endpos;
        char* start = block->buff + (begin % blockSize);
        if(destLen - len < static_cast<size_t>(end - start)) {
            end = start + destLen - len;
        }
        ::memcpy(dest + len, start, end - start);
        begin += static_cast<int>(end - start);
        len += static_cast<int>(end - start);
        block = block->next;
    }
    return len;
}

void Buffer::append(const char* data, size_t dataLen) {
    if(writeableBytes() < dataLen)
        makeSpace(dataLen);

    size_t len = 0;
    size_t begin = writeIdx_;
    size_t beginIdx = begin / blockSize;
    BufferBlock* block = head_;
    while(beginIdx) {
        block = block->next;
        beginIdx--;
    }
    while(len < dataLen) {
        char* dest = block->buff + (begin % blockSize);
        char* start = const_cast<char*>(data + len);
        char* end = const_cast<char*>(data + dataLen);
        if(dataLen - len > blockSize - (begin % blockSize)) {
            end = start + blockSize - (begin % blockSize);
        }

        memcpy(dest, start, end - start);
        len += static_cast<int>(end - start);
        begin += static_cast<int>(end - start);
        block = block->next;
    }
    writeIdx_ += len;
}

void Buffer::append(BufferIterator beginIt, BufferIterator endIt) {
    size_t dataLen = endIt - beginIt;
    if(writeableBytes() < dataLen)
        makeSpace(dataLen);

    BufferBlock* src = beginIt.buff_->head();
    size_t begin = beginIt.pos_, end = endIt.pos_;
    size_t srcIdx = begin / blockSize;
    while(srcIdx) {
        src = src->next;
        srcIdx--;
    }

    while(begin < end) {
        size_t len = 0;
        if((end - begin) <= (blockSize - (begin % blockSize))) {
            append(src->buff + (begin % blockSize), end - begin);
            len = end - begin;
        } else {
            append(src->buff + begin % blockSize, blockSize - (begin % blockSize));
            len = blockSize - (begin % blockSize);
        }
        begin += len;
    }
}

void Buffer::retrieveBytes(size_t bytes) {
    if(bytes >= readableBytes()) {
        retrieve();
        return;
    }

    readIdx_ += bytes;
    while(readIdx_ >= blockSize) {
        head_ = head_->next;
        readIdx_ -= blockSize;
        writeIdx_ -= blockSize;
    }
}

void Buffer::shrink(size_t reserve) {
    size_t r = blockSize + readableBytes() > reserve ? readableBytes() + blockSize : reserve;
    r = (r + blockSize - 1) & (~(blockSize - 1));
    if(r >= size_) {
        return;
    }
    while(size_ > r ) {
        BufferBlock* block = head_->prev;
        block->prev->next = head_;
        head_->prev = block->prev;
        loop_->free(block);
        size_ -= blockSize;
    }
}

void Buffer::reset(EventLoop* loop) {
    while(head_->next != head_) {
        BufferBlock* block = head_->next;
        head_->next = block->next;
        loop_->free(block);
    }
    loop_->free(head_);

    loop_ = loop;
    size_ = 0;
    head_ = NULL;
    writeIdx_ = 0;
    readIdx_ = 0;
    BufferBlock* block = loop_->allocate();
    if(block){
        insert(block);
        size_ += blockSize;
    }
}