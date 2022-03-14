#ifndef __NET_TCPCONNECTIONPOOL__
#define __NET_TCPCONNECTIONPOOL__

#include "InetAddress.h"
#include <list>
#include <string>

namespace netlib {
namespace net {

class EventLoop;
class TcpConnection;

class TcpConnectionPool {
public:
 TcpConnectionPool()
        : size_(0) {
 }
 ~TcpConnectionPool();

 TcpConnection* allocate(EventLoop* loop, std::string& name, int sock, InetAddress& localAddr, InetAddress& peerAddr);
 void free(TcpConnection* conn);

private:
 std::list<TcpConnection*> connections_;
 size_t size_;
};

}
}
#endif