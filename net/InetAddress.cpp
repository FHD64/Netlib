#include "StringPiece.h"
#include "InetAddress.h"
#include "Logger.h"

#include <string.h>
#include <endian.h>
#include <arpa/inet.h>

using namespace netlib;
using namespace netlib::net;

InetAddress::InetAddress(uint16_t port, bool loopback, bool ipv6) {
    if(ipv6) {
        ::bzero(&addr6_, sizeof addr6_);
        addr6_.sin6_family = AF_INET6;
        addr6_.sin6_addr = loopback ? in6addr_loopback : in6addr_any;
        addr6_.sin6_port = ::htobe16(port);
    } else {
        ::bzero(&addr_, sizeof addr_);
        addr_.sin_family = AF_INET;
        addr_.sin_addr.s_addr = ::htobe32(loopback ? INADDR_LOOPBACK : INADDR_ANY);
        addr_.sin_port = ::htobe16(port);
    }
}

InetAddress::InetAddress(StringPiece ip, uint16_t port, bool ipv6) {
    if(ipv6 || strchr(ip.data(), ':')) {
        ::bzero(&addr6_, sizeof addr6_);
        addr6_.sin6_family = AF_INET6;
        addr6_.sin6_port = ::htobe16(port);
        if (::inet_pton(AF_INET6, ip.data(), &(addr6_.sin6_addr)) <= 0) {
            LOG_SYSERR << "InetAddress::InetAddressv6";
        }
    } else {
        ::bzero(&addr_, sizeof addr_);
        addr_.sin_family = AF_INET;
        addr_.sin_port = ::htobe16(port);
        if(::inet_pton(AF_INET, ip.data(), &(addr_.sin_addr)) <= 0) {
            LOG_SYSERR << "InetAddress::InetAddressv4";
        }
    }
}

string InetAddress::toIp() const{
    char buf[64] = "";
    const struct sockaddr* sa = getSockAddr();
    if(sa->sa_family == AF_INET) {
        ::inet_ntop(AF_INET, &(addr_.sin_addr), buf, static_cast<socklen_t>(sizeof buf));
    } else if(sa->sa_family == AF_INET6) {
        ::inet_ntop(AF_INET6, &(addr6_.sin6_addr), buf, static_cast<socklen_t>(sizeof buf));
    }
      
    return buf;
}

string InetAddress::toIpPort() const{
    char buf[64] = "";
    const struct sockaddr* sa = getSockAddr();
    uint16_t port = ::be16toh(addr_.sin_port);
    if(sa->sa_family == AF_INET) {
        ::inet_ntop(AF_INET, &(addr_.sin_addr), buf, static_cast<socklen_t>(sizeof buf));
        size_t end = ::strlen(buf);
        snprintf(buf + end, (sizeof buf) - end, ":%u", port);
    } else if(sa->sa_family == AF_INET6) {
        buf[0] = '[';
        ::inet_ntop(AF_INET6, &(addr6_.sin6_addr), buf + 1, static_cast<socklen_t>((sizeof buf) - 1));
        size_t end = ::strlen(buf);
        snprintf(buf + end, (sizeof buf) - end, "]:%u", port);
    } 
    return buf;
}