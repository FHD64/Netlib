#include "Acceptor.h"
#include "Logger.h"
#include <fcntl.h>

using namespace netlib;
using namespace netlib::net;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenaddr, bool reuseport)
    : loop_(loop),
      acceptsock_(listenaddr.family()),
      acceptchannel_(loop, acceptsock_.fd()),
      listening_(false),
      idlefd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {
    acceptsock_.setReuseAddr(true);
    acceptsock_.setReusePort(reuseport);
    acceptsock_.bind(listenaddr);
    acceptchannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor() {
    acceptchannel_.disableAll();
    acceptchannel_.remove();
    ::close(idlefd_);
}

void Acceptor::listen() {
    listening_ = true;
    acceptsock_.listen();
    acceptchannel_.enableReading();
}

void Acceptor::handleRead() {
    InetAddress peeraddr;

    int connfd = acceptsock_.accpet(&peeraddr);
    if(connfd >= 0) {
        if(newconnectioncallback_) {
            newconnectioncallback_(connfd, peeraddr);
        } else {
            ::close(connfd);
        }
    } else {
        LOG_SYSERR << "in Acceptor::handlread";

        //当前进程文件描述符已分配完毕，直接关闭
        if(errno == EMFILE) {
            ::close(idlefd_);
            idlefd_ = ::accept(acceptsock_.fd(), NULL, NULL);
            ::close(idlefd_);
            idlefd_ = ::open("dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}