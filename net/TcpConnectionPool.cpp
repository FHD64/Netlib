#include "TcpConnectionPool.h"
#include "TcpConnection.h"

using namespace netlib;
using namespace netlib::net;

 TcpConnectionPool::~TcpConnectionPool() {
     for(TcpConnection* ptr : connections_) {
         delete ptr;
     }
 }

TcpConnection* TcpConnectionPool::allocate(EventLoop* loop, std::string& name, int sock, InetAddress& localAddr, InetAddress& peerAddr) {
     if(size_ > 0) {
         TcpConnection* ptr = connections_.front();
         ptr->init(loop, name, sock, localAddr, peerAddr);
         connections_.pop_front();
         size_--;
         return ptr;
     }
     return new TcpConnection(loop, name, sock, localAddr, peerAddr);
 }

 void TcpConnectionPool::free(TcpConnection* conn) {
     conn->socket_->reset();
     conn->channel_->reset();
     connections_.push_back(conn);
     size_++;
 }