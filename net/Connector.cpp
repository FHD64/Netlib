#include "Connector.h"
#include "LogStream.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"

using namespace netlib;
using namespace netlib::net;

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
    : loop_(loop),
      serverAddr_(serverAddr),
      connect_(false),
      state_(kDisconnected),
      retryMs_(kInitRetryMs) {
    LOG_DEBUG << "ctor[" << this << "]";
}

Connector::~Connector() {
    LOG_DEBUG << "dtor[" << this << "]";
}

void Connector::start() {
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop, shared_from_this()));
}

void Connector::startInLoop() {
    if(connect_) {
        connect();
    } else {
        LOG_DEBUG << "is Disconnected";
    }
}

void Connector::stop() {
    connect_ = false;
    loop_->queueInLoop(std::bind(&Connector::stopInLoop, shared_from_this()));
}

void Connector::stopInLoop() {
    if(state_ == kConnecting) {
        state_ = kDisconnected;
        int sockfd = removeAndResetChannel();
        retry(sockfd);
    }
}

void Connector::connect() {
    int sockfd = ::socket(serverAddr_.family(), SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if(sockfd < 0) {
        LOG_SYSFATAL << "Connector::create socket fail";
    }

    int ret = ::connect(sockfd, serverAddr_.getSockAddr(), static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
    int savederrno = (ret == 0) ? 0 : errno;
    switch(savederrno) {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
            connecting(sockfd);
            break;
        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            retry(sockfd);
            break;
        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            LOG_SYSERR << "Connector::connect error" << savederrno;
            if(::close(sockfd) < 0) {
                LOG_SYSERR << "Connector::connect(), close fail";
            }
            break;
        default:
            LOG_SYSERR << "Connector::connect(), unexpected error";
            if(::close(sockfd) < 0) {
                LOG_SYSERR << "Connector::connect(), close fail";
            }
            break;
    }
}

void Connector::restart() {
    state_ = kDisconnected;
    retryMs_ = kInitRetryMs;
    start();
}

void Connector::connecting(int sockfd) {
    state_ = kConnecting;
    channel_.reset(new Channel(loop_, sockfd));

    channel_->setWriteCallback(std::bind(&Connector::handleWrite, shared_from_this()));
    channel_->setErrorCallback(std::bind(&Connector::handleError, shared_from_this()));
    channel_->enableWriting();
}

int Connector::removeAndResetChannel() {
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();

    loop_->queueInLoop(std::bind(&Connector::resetChannel, shared_from_this()));
    return sockfd;
}

void Connector::resetChannel() {
    channel_.reset();
}

void Connector::handleWrite() {
    if(state_ != kConnecting) {
        LOG_WARN << "Connector::handleWrite, err state ";
    }

    int sockfd = removeAndResetChannel();
    int err;
    socklen_t optlen = static_cast<socklen_t>(sizeof err);
    if(::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &optlen) < 0) {
        err = errno;
    }

    if(err) {
        LOG_WARN << "Connector::handleWrite, err = " << err;
        retry(sockfd);
    } else if (Socket::isSelfConnect(sockfd)) {
        LOG_WARN << "Connector::handleWrite, self Connect";
        retry(sockfd);
    } else {
        state_ = kConnected;
        if(connect_) {
            newConnectionCallback_(sockfd);
        } else {
            if(::close(sockfd) < 0) {
                LOG_SYSERR << "Connector::handleWrite, close socket";
            }
        }
    }
}

void Connector::handleError() {
    LOG_ERROR << "Connector::handleError";
    if(state_ != kConnecting) {
        return;
    }

    int sockfd = removeAndResetChannel();
    int err;
    socklen_t optlen = static_cast<socklen_t>(sizeof err);
    if(::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &optlen) < 0) {
        err = errno;
    }
    LOG_TRACE << "Connector::handleError, err = " << err;
    retry(sockfd);
}

void Connector::retry(int sockfd) {
    if(::close(sockfd) < 0) {
        LOG_SYSERR << "Connector::retry, close faile";
    }
    state_ = kDisconnected;

    if(connect_) {
        loop_->runAfter(retryMs_ / 1000.0, std::bind(&Connector::startInLoop, shared_from_this()));
        //二进制指数退避
        retryMs_ = retryMs_ * 2 < kMaxRetryMs ? retryMs_ * 2 : kMaxRetryMs;
    }
}