#include "InetAddress.h"
#include "Logger.h"
#include <string>

using std::string;
using netlib::net::InetAddress;

int testCase = 0;
int rightCase = 0;
#define CHECK_EUQAL(a, b) \
    do { \
        testCase++; \
        if((a) == (b)) { \
            rightCase++; \
            printf("right case : func %s, Line %d\n", __func__, __LINE__); \
        } else { \
            printf("wrong case : func %s, Line %d\n", __func__, __LINE__); \
        } \
    } while(0)

void testInetAddress() {
    InetAddress addr0(1234);
    CHECK_EUQAL(addr0.toIp(), string("0.0.0.0"));
    CHECK_EUQAL(addr0.toIpPort(), string("0.0.0.0:1234"));
    CHECK_EUQAL(addr0.port(), 1234);

    InetAddress addr1(4321, true);
    CHECK_EUQAL(addr1.toIp(), string("127.0.0.1"));
    CHECK_EUQAL(addr1.toIpPort(), string("127.0.0.1:4321"));
    CHECK_EUQAL(addr1.port(), 4321);

    InetAddress addr2("1.2.3.4", 8888);
    CHECK_EUQAL(addr2.toIp(), string("1.2.3.4"));
    CHECK_EUQAL(addr2.toIpPort(), string("1.2.3.4:8888"));
    CHECK_EUQAL(addr2.port(), 8888);

    InetAddress addr3("255.254.253.252", 65535);
    CHECK_EUQAL(addr3.toIp(), string("255.254.253.252"));
    CHECK_EUQAL(addr3.toIpPort(), string("255.254.253.252:65535"));
    CHECK_EUQAL(addr3.port(), 65535);
}

void testInet6Address() {
    InetAddress addr0(1234, false, true);
    CHECK_EUQAL(addr0.toIp(), string("::"));
    CHECK_EUQAL(addr0.toIpPort(), string("[::]:1234"));
    CHECK_EUQAL(addr0.port(), 1234);

    InetAddress addr1(1234, true, true);
    CHECK_EUQAL(addr1.toIp(), string("::1"));
    CHECK_EUQAL(addr1.toIpPort(), string("[::1]:1234"));
    CHECK_EUQAL(addr1.port(), 1234);

    InetAddress addr2("2001:db8::1", 8888, true);
    CHECK_EUQAL(addr2.toIp(), string("2001:db8::1"));
    CHECK_EUQAL(addr2.toIpPort(), string("[2001:db8::1]:8888"));
    CHECK_EUQAL(addr2.port(), 8888);

    InetAddress addr3("fe80::1234:abcd:1", 8888);
    CHECK_EUQAL(addr3.toIp(), string("fe80::1234:abcd:1"));
    CHECK_EUQAL(addr3.toIpPort(), string("[fe80::1234:abcd:1]:8888"));
    CHECK_EUQAL(addr3.port(), 8888);
}

int main() {
    printf("test begin...\n");
    testInetAddress();
    testInet6Address();
    printf("test end, test case : %d, right case : %d \n", testCase, rightCase);
}