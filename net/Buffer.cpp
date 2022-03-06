#include "Buffer.h"
#include <sys/uio.h>
#include <errno.h>
#include <unistd.h>

using namespace netlib;

void net::Buffer::makeSpace(size_t len) {
    if (writeableBytes() + prependBytes() < len + kprepend)
        buffer_.resize(writeidx_ + len);
    
    //无论是否进行resize，均需重新移动数据释放空间
    size_t readable = readableBytes();
    std::copy(begin() + readidx_, begin() + writeidx_, begin() + kprepend);
    readidx_ = kprepend;
    writeidx_ = readidx_ + readable;
}

ssize_t net::Buffer::readFd(int fd, int* savedErrno) {
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writeable = writeableBytes();

    vec[0].iov_base = begin() + writeidx_;
    vec[0].iov_len = writeable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    ssize_t n = ::readv(fd, vec, 2);
    if(n < 0) {
        *savedErrno = errno;
    } else if (static_cast<size_t>(n) <= writeable) {
        writeidx_ += n;
    } else {
        writeidx_ = buffer_.size();
        append(extrabuf, n - writeable);
    }

    return n;
}