#ifndef __NET_INETADDRESS__
#define __NET_INETADDRESS__

#include "StringPiece.h"
#include <netinet/in.h>

namespace netlib {

namespace net {
//支持ipv6
class InetAddress {
 public:
  explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false, bool ipv6 = false);
  InetAddress(StringPiece ip, uint16_t port, bool ipv6 = false);
  explicit InetAddress(const struct sockaddr_in& addr) : addr_(addr) {
  }
  explicit InetAddress(const struct sockaddr_in6& addr) : addr6_(addr) {
  }
  
  sa_family_t family() const {
      return addr_.sin_family;
  }
  uint16_t portNetEndian() const {
      return addr_.sin_port;
  }
  uint16_t port() const {
      return ::be16toh(portNetEndian());
  }
  uint32_t ipv4NetEndian() const {
      return addr_.sin_addr.s_addr;
  }

  //端口、ip地址类型转换
  const struct sockaddr* getSockAddr() const {
      return reinterpret_cast<const struct sockaddr*>(&addr6_);
  }
  void setSockAddrInet6(const struct sockaddr_in6& addr6) {
      addr6_ = addr6;
  }
  string toIp() const;
  string toIpPort() const;

  //设置IPv6 ScopeId
  void setScopeId(uint32_t sid) {
      if(family() == AF_INET6)
          addr6_.sin6_scope_id = sid;
  }
 private:
  union {
      struct sockaddr_in addr_;
      struct sockaddr_in6 addr6_;
  };
};
}
}
#endif