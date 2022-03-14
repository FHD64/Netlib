#include "TcpClient.h"
#include "Callback.h"
#include "TcpConnection.h"
#include "Logger.h"
#include "EventLoop.h"
#include "Connector.h"

#include <functional>

using namespace netlib;
using namespace netlib::net;
using namespace std::placeholders;

namespace netlib {

namespace net {

void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn) {
    loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}
}
}

TcpClient::TcpClient(EventLoop* loop, const InetAddress& serveraddr, const string& name)
        : loop_(loop),
          connector_(new Connector(loop, serveraddr)),
          name_(name),
          connectionCallback_(defaultConnectionCallback),
          messageCallback_(defaultMessageCallback),
          retry_(false),
          connect_(true),
          nextid_(1) {
    if(loop_ == NULL) {
        LOG_FATAL << "TcpClient::ctor, loop is NULL";
    }

    connector_->setNewConnectionCallback(std::bind(&TcpClient::newConnection, this, _1));
    LOG_INFO << "TcpClient::ctor name[" << name_ << "]";
}

TcpClient::~TcpClient() {
    LOG_INFO << "TcpClient::dtor, name[" << name_ << "]";

    TcpConnectionPtr conn;
    bool unique = false;
    {
        MutexLock lock(&mutex_);
        unique = connection_.unique();
        conn = connection_;
    }

    if(conn) {
        CloseCallback cb = std::bind(&netlib::net::removeConnection, loop_, _1);
        loop_->runInLoop(std::bind(&TcpConnection::setCloseCallback, conn, cb));
        if(unique) {
            conn->forceClose();
        }
    } else {
        connector_->stop();
    }
}

void TcpClient::connect() {
    connect_ = true;
    connector_->start();
}

void TcpClient::disconnect() {
    connect_ = false;

    {
        MutexLock lock(&mutex_);
        if(connection_) {
            connection_->shutdown();
        }
    }
}

void TcpClient::stop() {
    connect_ = false;
    connector_->stop();
}

void TcpClient::newConnection(int sockfd) {
    InetAddress peeraddr(Socket::getPeerAddr(sockfd));
    char buf[32];

    snprintf(buf, sizeof buf, ":%s#%d", peeraddr.toIpPort().c_str(), nextid_);
    ++nextid_;
    string conname = name_ + buf;

    InetAddress localaddr(Socket::getLocalAddr(sockfd));
    TcpConnectionPtr conn(loop_->alloConnection(loop_, conname, sockfd, localaddr, peeraddr), std::bind(&EventLoop::freeConnection, loop_, _1));
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpClient::removeConnection, this, _1));
    {
        MutexLock lock(&mutex_);
        connection_ = conn;
    }
    conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn) {
    {
        MutexLock lock(&mutex_);
        connection_.reset();
    }

    loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    if(retry_ && connect_) {
        connector_->restart();
    }
}