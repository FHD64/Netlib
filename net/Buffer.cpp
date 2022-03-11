#include "Buffer.h"
#include "Logger.h"
#include <errno.h>
#include <unistd.h>
#include<cstring>

namespace netlib {
namespace net {

BufferIterator operator-(BufferIterator it, int dis) {
    if(dis < 0) {
        it += -dis;
        return it;
    }

    size_t remain = static_cast<size_t>(dis);
    while(it.buff_ && remain != 0) {
        if(it.pos_ + 1 > remain) {
            it.pos_ -= remain;
            break;
        } else {
            if(it.buff_->prev) {
                remain -= it.pos_+1;
                it.pos_ = blockSize-1;
                it.buff_ = it.buff_->prev;
            } else {
                it.pos_ = 0;
                break;
            }
        }
    }
    return it;
}

BufferIterator operator+(BufferIterator it, int dis) {
    if(dis < 0) {
        it -= -dis;
        return it;
    }
    if(it.buff_->next && it.pos_ == blockSize) {
        it.buff_ = it.buff_->next;
        it.pos_ = 0;
    }
    size_t remain = static_cast<size_t>(dis);
    while(it.buff_ && remain != 0) {
        if((blockSize - it.pos_) > remain) {
            it.pos_ += remain;
            break;
        } else {
            if(it.buff_->next) {
                remain -= blockSize - it.pos_;
                it.pos_ = 0;
                it.buff_ = it.buff_->next;
            }else {
                it.pos_ = blockSize;
                break;
            }
        }
    }
    return it;
}

size_t operator-(BufferIterator it1, BufferIterator it2) {
    BufferIterator big, small;
    if(it1 >= it2) {
        big = it1;
        small = it2;
    } else {
        big = it2;
        small = it1;
    }

    int dis = 0;
    BufferBlock* temp = small.buff_;
    while(temp && temp != big.buff_) {
        temp = temp->next;
        dis++;
    }
    return static_cast<size_t>(dis * blockSize + big.pos_ - small.pos_);

}

bool operator<(BufferIterator it1, BufferIterator it2) {
    if(it1.buff_ == it2.buff_) {
        return it1.pos_ < it2.pos_;
    } else {
        BufferBlock* next = it1.buff_->next;
        while(next) {
            if(next == it2.buff_) {
                return true;
            }
            next = next->next;
        }
    }
    return false;
}
}
}

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
    size_ += expansion;
    while (expansion > 0) {
        insertTail(loop_->allocate());
        expansion -= blockSize;
    }
}

Buffer::~Buffer() {
    while(head_) {
        BufferBlock* block = head_;
        head_ = head_->next;
        loop_->free(block);
    }
}

size_t Buffer::makeIov(int* iovsize, BufferIterator begin, BufferIterator end) {
    *iovsize = 0;
    size_t size = end - begin;

    while(*iovsize < maxIov && begin < end) {
        if(begin.buff_ == end.buff_) {
            iov_[*iovsize].iov_base = begin.buff_->buff;
            iov_[*iovsize].iov_len = end.pos_ - begin.pos_;
            begin += end.pos_ - begin.pos_;
        }else {
            iov_[*iovsize].iov_base = begin.buff_->buff;
            iov_[*iovsize].iov_len = blockSize - begin.pos_;
            begin += blockSize - begin.pos_;
        } 
        (*iovsize)++;
    }

    return size;
}

ssize_t Buffer::readFd(int fd, int* savedErrno) {
    char extrabuf[65536];
    int iovsize = 0;
    size_t len = makeIov(&iovsize, writeIt_, BufferIterator(tail_, blockSize));
    
    if(iovsize < maxIov) {
        iov_[iovsize].iov_base = extrabuf;
        iov_[iovsize].iov_len = sizeof extrabuf;
        iovsize++;
    }

    ssize_t n = ::readv(fd, iov_, iovsize);
    if(n < 0) {
        *savedErrno = errno;
    }else if(static_cast<size_t>(n) <= len) {
        writeIt_ += static_cast<int>(n);
    }else {
        writeIt_ += len;
        append(extrabuf, n - len);
    }

    return n;
}

ssize_t Buffer::writeFd(int fd) {
    int iovsize = 0;
    size_t len = makeIov(&iovsize, readIt_, writeIt_);
    len = len;
    ssize_t n = ::writev(fd, iov_, iovsize);
    return n;
}

size_t Buffer::copyToUser(char* dest, size_t destLen) {
    size_t len = 0;
    BufferIterator it = readIt_;
    while((it < writeIt_) && (len < destLen)) {
        size_t endpos = (it.buff_ == writeIt_.buff_) ? writeIt_.pos_ : blockSize;
        char* end = it.buff_->buff + endpos;
        char* start = it.buff_->buff + it.pos_;
        if(destLen - len < static_cast<size_t>(end - start)) {
            end = start + destLen - len;
        }
        ::memcpy(dest + len, start, end - start);
        it += static_cast<int>(end - start);
        len += static_cast<int>(end - start);
    }
    return len;
}

void Buffer::append(const char* data, size_t dataLen) {
    if(writeableBytes() < dataLen)
        makeSpace(dataLen);

    size_t len = 0;
    BufferIterator it = writeIt_;
    while(len < dataLen) {
        char* dest = it.buff_->buff + it.pos_;
        char* start = const_cast<char*>(data + len);
        char* end = const_cast<char*>(data + dataLen);
        if(dataLen - len > blockSize - it.pos_) {
            end = start + blockSize - it.pos_;
        }

        memcpy(dest, start, end - start);
        len += static_cast<int>(end - start);
        it += static_cast<int>(end - start);
    }
    writeIt_ += len;
    appendsize_ += len;
}

void Buffer::append(BufferIterator begin, BufferIterator end) {
    size_t dataLen = end - begin;
    if(writeableBytes() < dataLen)
        makeSpace(dataLen);
    
    BufferIterator it = begin;
    while(it < end) {
        size_t len = 0;
        if(it.buff_ == end.buff_) {
            append(it.buff_->buff + it.pos_, end.pos_ - it.pos_);
            len = end.pos_ - it.pos_;
        } else {
            append(it.buff_->buff + it.pos_, blockSize - it.pos_);
            len = blockSize - it.pos_;
        }
        it += len;
    }
}

void Buffer::retrieveBytes(size_t bytes) {
    if(bytes >= readableBytes()) {
        retrieve();
        return;
    }

    readIt_ += bytes;
    if(readIt_.buff_ != head_) {
        insertTail(removeHead());
    }
    retrievesize_ += bytes;
}

void Buffer::shrink(size_t reserve) {
    size_t r = blockSize + readableBytes() > reserve ? readableBytes() + blockSize : reserve;
    r = (r + blockSize - 1) & (~(blockSize - 1));
    if(r >= size_) {
        return;
    }
    while(size_ > r) {
        loop_->free(removeTail());
        size_ -= blockSize;
    }
}