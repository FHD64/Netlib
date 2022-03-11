#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "StringPiece.h"
#include "EventLoop.h"

#include <iostream>
using namespace netlib;
using namespace netlib::net;
namespace netlib {

namespace net {

void defaultConnectionCallback(const TcpConnectionPtr& conn) {
    LOG_TRACE << conn->localAddress().toIpPort() 
              << " -> "
              << conn->peerAddress().toIpPort()
              << " is "
              << (conn->connected() ? "CONNECTED" : "DOWN");
}

void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer* buf) {
    buf->retrieve();
}   

}
}


TcpConnection::TcpConnection(EventLoop* loop, const string& name, int fd, const InetAddress& localAddr, const InetAddress& peerAddr) 
            : name_(name),
              loop_(loop),
              state_(kConnecting),
              reading_(true),
              socket_(new Socket(fd)),
              channel_(new Channel(loop, fd)),
              localAddr_(localAddr),
              peerAddr_(peerAddr),
              //64KB
              highWaterMark_(64 * 1024 * 1024),
              inputBuffer_(loop_),
              outputBuffer_(loop_) {
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    LOG_DEBUG << "TcpConnection::ctor[" <<  name_ << "] at " << this << " sockfd=" << fd;
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
    LOG_DEBUG << "TcpConnection::dtor[" 
              <<  name_ 
              << "] at " 
              << this
              << " fd=" << channel_->fd()
              << " state=" << stateToString();
}

const char* TcpConnection::stateToString() const {
    switch(state_) {
        case kDisconnected:
            return "kDisconnected";
        case kConnecting:
            return "kConnecting";
        case kConnected:
            return "kConnected";
        case kDisconnecting:
            return "kDisconnecting";
        default:
            return "unknown state";
    }
}

bool TcpConnection::getTcpInfo(struct tcp_info* tcpi) const {
    return socket_->getTcpInfo(tcpi);
}

void TcpConnection::send(const StringPiece& message) {
    if(state_ == kConnected) {
        if(loop_->isInLoopThread()) {
            sendInLoop(message);
        } else {
            void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(std::bind(fp, this, message));
        }
    }
}

void TcpConnection::send(const void* data, int len) {
    send(StringPiece(static_cast<const char*>(data), len));
}

void TcpConnection::sendInLoopAndFree(const void* data, size_t len) {
    sendInLoop(data, len);
    free(reinterpret_cast<char*>(const_cast<void*>(data)));
}

void TcpConnection::send(Buffer* message) {
    if(state_ == kConnected) {
        if(loop_->isInLoopThread()) {
            sendInLoop(message);
            message->retrieve();
        } else {
            size_t len = message->readableBytes();
            char *msg = reinterpret_cast<char*>(malloc(sizeof(char) * len));
            message->copyToUser(msg, len);
            void (TcpConnection::*fp)(const void* data, size_t len) = &TcpConnection::sendInLoopAndFree;
            loop_->runInLoop(std::bind(fp, this, msg, len));
            message->retrieve();
        }
    }
}

void TcpConnection::sendInLoop(const void* data, size_t len) {
    ssize_t nwrote = 0;
    size_t nremain = len;
    bool fatalerror = false;

    if(state_ == kDisconnected) {
        LOG_WARN << "disconnected, cann't write data";
        return;
    }

    //尝试直接写入
    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = ::write(channel_->fd(), data, len);
        if(nwrote >= 0) {
            nremain = len - nwrote;
            if(nremain == 0 && writeCompleteCallback_) {
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }else {
            nwrote = 0;
            if(errno != EWOULDBLOCK) {
                LOG_SYSERR << "TcpConnection::sendInLoop";
                if(errno == EPIPE || errno == ECONNRESET) {
                    fatalerror = true;
                }
            }
        }
    }

    //若当前未一次性写完
    if(!fatalerror && nremain > 0) {
        size_t oldlen = outputBuffer_.readableBytes();
        if(oldlen + nremain >= highWaterMark_
           && oldlen < highWaterMark_
           && highWaterMarkCallback_) {
               loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldlen + nremain));
        }

        outputBuffer_.append(static_cast<const char*>(data) + nwrote, nremain);
        if(!channel_->isWriting()) {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::sendInLoop(Buffer* data) {
    ssize_t nwrote = 0;
    size_t len = data->readableBytes();
    size_t nremain = len;
    bool fatalerror = false;

    if(state_ == kDisconnected) {
        LOG_WARN << "disconnected, cann't write data";
        return;
    }

    //尝试直接写入
    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = data->writeFd(channel_->fd());
        if(nwrote >= 0) {
            nremain = len - nwrote;
            if(nremain == 0 && writeCompleteCallback_) {
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }else {
            nwrote = 0;
            if(errno != EWOULDBLOCK) {
                LOG_SYSERR << "TcpConnection::sendInLoop";
                if(errno == EPIPE || errno == ECONNRESET) {
                    fatalerror = true;
                }
            }
        }
    }

    //若当前未一次性写完
    if(!fatalerror && nremain > 0) {
        size_t oldlen = outputBuffer_.readableBytes();
        if(oldlen + nremain >= highWaterMark_
           && oldlen < highWaterMark_
           && highWaterMarkCallback_) {
               loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldlen + nremain));
        }

        outputBuffer_.append(data->begin()+static_cast<size_t>(nwrote), data->end());
        if(!channel_->isWriting()) {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::sendInLoop(const StringPiece& message) {
    sendInLoop(message.data(), message.size());
}

void TcpConnection::shutdown() {
    if(state_ == kConnected) {
        state_ = kDisconnecting;
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, shared_from_this()));
    }
}

void TcpConnection::shutdownInLoop() {
    if(!channel_->isWriting()) {
        socket_->shutdownWrite();
    }
}

void TcpConnection::forceCloseInLoop() {
    if(state_ == kConnected || state_ == kDisconnecting) {
        handleClose();
    }
}

void TcpConnection::forceClose() {
    if(state_ == kConnected || state_ == kDisconnecting) {
        state_ = kDisconnecting;
        loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}

void TcpConnection::forceCloseWithDelay(double seconds) {
    if(state_ == kConnected || state_ == kDisconnecting) {
        state_ = kDisconnecting;
        loop_->runAfter(seconds, std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}

void TcpConnection::startRead() {
    loop_->runInLoop(std::bind(&TcpConnection::startReadInLoop, shared_from_this()));
}

void TcpConnection::startReadInLoop() {
    if(!reading_ || !channel_->isReading()) {
        channel_->enableReading();
        reading_ = true;
    }
}

void TcpConnection::stopRead() {
    loop_->runInLoop(std::bind(&TcpConnection::stopReadInLoop, shared_from_this()));
}

void TcpConnection::stopReadInLoop() {
    if(reading_ || channel_->isReading()) {
        channel_->disableReading();
        reading_ = false;
    }
}

void TcpConnection::connectEstablished() {
    state_ = kConnected;

    channel_->tie(shared_from_this());
    channel_->enableReading();
    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed() {
    if(state_ == kConnected) {
        state_ = kDisconnected;
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }

    channel_->remove();
}

void TcpConnection::handleRead() {
    int savederrno = 0;

    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savederrno);
    if(n > 0) {
        messageCallback_(shared_from_this(), &inputBuffer_);
    }else if(n == 0) {
        handleClose();
    }else {
        errno = savederrno;
        LOG_SYSERR << "TcpConnection::handleRead";
        handleError();
    }
}

void TcpConnection::handleWrite() {
    if(channel_->isWriting()) {
        ssize_t n = outputBuffer_.writeFd(channel_->fd());

        if(n > 0) {
            outputBuffer_.retrieveBytes(n);
            if(outputBuffer_.readableBytes() == 0) {
                channel_->disableWriting();
                if(writeCompleteCallback_) {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }

                if(state_ == kDisconnecting) {
                    shutdownInLoop();
                }
            }
        }else {
            LOG_SYSERR << "TcpConnection::handleWrite";
        }
    }else {
        LOG_TRACE << "Connection is done, fd = " << channel_->fd();
    }
}

void TcpConnection::handleClose() {
    LOG_TRACE << "fd = " << channel_->fd() << " state = " << stateToString();

    state_ = kDisconnected;
    channel_->disableAll();

    TcpConnectionPtr guard(shared_from_this());
    connectionCallback_(guard);
    closeCallback_(guard);
}

void TcpConnection::handleError() {
    int err;
    char errnobuf[512];
    socklen_t optlen = static_cast<socklen_t>(sizeof err);

    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &err, &optlen) < 0) {
        err = errno;
    }

    LOG_ERROR << "TcpConnection::handleError [" << name_ << "] - SO_ERROR = " << err << " " << strerror_r(err, errnobuf, sizeof errnobuf);
}

