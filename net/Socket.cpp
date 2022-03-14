#include "Socket.h"
#include "Logger.h"
#include "Buffer.h"
#include <sys/socket.h>

using namespace netlib;
using namespace netlib::net;

int Socket::accpet(InetAddress* peeraddr) {
    struct sockaddr_in6 addr;
    socklen_t addrlen = static_cast<socklen_t>(sizeof addr);
    ::bzero(&addr, addrlen);
    int connfd = ::accept4(fd_, reinterpret_cast<struct sockaddr*>(&addr), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if(connfd >= 0) {
        peeraddr->setSockAddrInet6(addr);
    }
    
    return connfd;
}

struct sockaddr_in6 Socket::getPeerAddr(int sockfd) {
    struct sockaddr_in6 peeraddr;
    memset(&peeraddr, 0, sizeof peeraddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
    if (::getpeername(sockfd, reinterpret_cast<struct sockaddr*>(&peeraddr), &addrlen) < 0) {
      LOG_SYSERR << "Socket::getPeerAddr";
    }
    return peeraddr;
}

struct sockaddr_in6 Socket::getLocalAddr(int sockfd) {
    struct sockaddr_in6 localaddr;
    memset(&localaddr, 0, sizeof localaddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
    if (::getsockname(sockfd, reinterpret_cast<struct sockaddr*>(&localaddr), &addrlen) < 0) {
      LOG_SYSERR << "Socket::getLocalAddr";
    }
    return localaddr;
}

bool Socket::isSelfConnect(int sockfd) {
    struct sockaddr_in6 localaddr = getLocalAddr(sockfd);
    struct sockaddr_in6 peeraddr = getPeerAddr(sockfd);
    if (localaddr.sin6_family == AF_INET) {
      const struct sockaddr_in* laddr4 = reinterpret_cast<struct sockaddr_in*>(&localaddr);
      const struct sockaddr_in* raddr4 = reinterpret_cast<struct sockaddr_in*>(&peeraddr);
      return laddr4->sin_port == raddr4->sin_port 
            && laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
    } else if (localaddr.sin6_family == AF_INET6) {
      return localaddr.sin6_port == peeraddr.sin6_port 
            && memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr, sizeof localaddr.sin6_addr) == 0;
    } else {
      return false;
    }
}

Socket::~Socket() {
    if ((fd_ > 0) && (::close(fd_) < 0)) {
        LOG_SYSERR << "sockets::close";
    }
}