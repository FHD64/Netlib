#ifndef __NET_SOCKET__
#define __NET_SOCKET__

#include "noncopyable.h"
#include "Logger.h"
#include "InetAddress.h"
#include <netinet/tcp.h>
#include <netinet/in.h>

namespace netlib {

namespace net {

class Socket : noncopyable {
 public:
  explicit Socket(sa_family_t family = AF_INET) 
        : fd_(::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP)) {
      if(fd_ < 0)
          LOG_SYSFATAL << "sockets:Socket(family)";
  }
  explicit Socket(int fd) : fd_(fd) {
  }
  ~Socket();

  int fd() const {
      return fd_;
  }
  //sock操作接口
  void bind(const InetAddress& localaddr) {
      int ret = ::bind(fd_, localaddr.getSockAddr(), static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
      if(ret < 0)
          LOG_SYSFATAL << "socket::bind";
  }
  void listen() {
      int ret = ::listen(fd_, SOMAXCONN);
      if(ret < 0) {
          LOG_SYSFATAL << "socket::listen";
      }
  }
  int accpet(InetAddress* peeraddr);

  //提供部分sock配置选项设置
  void shutdownWrite() {
      if(::shutdown(fd_, SHUT_WR) < 0) {
          LOG_SYSERR << "socket::shutdownWrite";
      }
  }
  void setTcpNoDelay(bool on) {
      int opt = on ? 1 : 0;
      ::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &opt, static_cast<socklen_t>(sizeof opt));
  }
  void setReuseAddr(bool on) {
      int opt = on ? 1 : 0;
      ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, static_cast<socklen_t>(sizeof opt));
  }
  void setReusePort(bool on) {
      int opt = on ? 1 : 0;
      ::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &opt, static_cast<socklen_t>(sizeof opt));
  }
  void setKeepAlive(bool on) {
      int opt = on ? 1 : 0;
      ::setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &opt, static_cast<socklen_t>(sizeof opt));
  }
  bool getTcpInfo(struct tcp_info* tcpi) const {
      socklen_t len = sizeof(*tcpi);
      memset(tcpi, 0, len);
      return ::getsockopt(fd_, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
  }
  static bool isSelfConnect(int sockfd);
  static struct sockaddr_in6 getLocalAddr(int sockfd);
  static struct sockaddr_in6 getPeerAddr(int sockfd);
 private:
  const int fd_;
};
}
}
#endif