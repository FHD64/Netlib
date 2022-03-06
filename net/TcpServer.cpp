#include "TcpServer.h"
#include "Acceptor.h"
#include "Callback.h"
#include "TcpConnection.h"
#include "EventLoop.h"
#include <sys/socket.h>

using namespace netlib;
using namespace netlib::net;
using namespace std::placeholders;

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenaddr, const string& name, bool reuseport)
        : loop_(loop),
          ipPort_(listenaddr.toIpPort()),
          name_(name),
          acceptor_(new Acceptor(loop, listenaddr, reuseport)),
          threadpool_(new EventLoopThreadPool(loop, name_)),
          connectionCallback_(defaultConnectionCallback),
          messageCallback_(defaultMessageCallback),
          nextid_(1) {
    
    if(loop_ == NULL){
        LOG_WARN << "TcpServer::Tcpserver, EventLoop == NULL";
    }

    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer() {
    for(auto& connection : connections_) {
        TcpConnectionPtr conn(connection.second);
        connection.second.reset();
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

void TcpServer::start() {
    if(started_.exchange(true) == false) {
        threadpool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress& peeraddr) {
    EventLoop* ioloop = threadpool_->getLoop();
    char buf[64];
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextid_);
    ++nextid_;

    string conname = name_ + buf;

    LOG_INFO << "TcpServer::newConnection [" 
             << name_
             << "] - new Connection ["
             << conname
             << "] from "
             << peeraddr.toIpPort();
    
    struct sockaddr_in6 localaddr;
    memset(&localaddr, 0, sizeof localaddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
    if(::getsockname(sockfd, reinterpret_cast<struct sockaddr*>(&localaddr), &addrlen) < 0) {
        LOG_SYSERR << "TcpServer::newConnection, getsockname err";
    }

    InetAddress addr(localaddr);
    TcpConnectionPtr conn(new TcpConnection(ioloop, conname, sockfd, addr, peeraddr));
    connections_[conname] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, _1));
    ioloop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn) {
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn) {
    LOG_INFO << "TcpServer::removeConnectionInLoop Server["
             << name_
             << "] - Connection["
             << conn->name()
             << "]";
    
    size_t n = connections_.erase(conn->name());
    if(n != 1) {
        LOG_SYSERR << "TcpServer::removeConnectionInLoop, erase wrong";
    }
    EventLoop* ioloop = conn->getLoop();
    ioloop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}