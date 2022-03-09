#include "Buffer.h"
#include <string>
#include <iostream>

using std::string;
using namespace netlib;
using namespace netlib::net;
using netlib::net::Buffer;

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

void testBufferAppendRetrieve() {
    EventLoop loop;
    Buffer buf(&loop);
    CHECK_EUQAL(buf.readableBytes(), 0);
    CHECK_EUQAL(buf.writeableBytes(), blockSize);

    const string str(200, 'x');
    buf.append(str);
    CHECK_EUQAL(buf.readableBytes(), str.size());
    CHECK_EUQAL(buf.writeableBytes(), blockSize - str.size());

    size_t len = 50;
    char* msg = reinterpret_cast<char*>(malloc(sizeof(char) * len));
    buf.copyToUser(msg, len);
    const string str2(msg, len);
    buf.retrieveBytes(50);
    free(msg);
    CHECK_EUQAL(str2.size(), 50);
    CHECK_EUQAL(buf.readableBytes(), str.size() - str2.size());
    CHECK_EUQAL(buf.writeableBytes(), blockSize - str.size());
    CHECK_EUQAL(str2, string(50, 'x'));

    buf.append(str);
    CHECK_EUQAL(buf.readableBytes(), 2*str.size() - str2.size());
    CHECK_EUQAL(buf.writeableBytes(), blockSize- 2 * str.size());

    len = buf.readableBytes();
    msg = reinterpret_cast<char*>(malloc(sizeof(char) * len));
    buf.copyToUser(msg, len);
    const string str3(msg,len);
    buf.retrieve();
    free(msg);
    CHECK_EUQAL(str3.size(), 350);
    CHECK_EUQAL(buf.readableBytes(), 0);
    CHECK_EUQAL(buf.writeableBytes(), blockSize);
    CHECK_EUQAL(str3, string(350, 'x'));
}

void testBufferGrow() {
    EventLoop loop;
    Buffer buf(&loop);
    buf.append(string(400, 'y'));
    CHECK_EUQAL(buf.readableBytes(), 400);
    CHECK_EUQAL(buf.writeableBytes(), blockSize-400);

    buf.retrieveBytes(50);
    CHECK_EUQAL(buf.readableBytes(), 350);
    CHECK_EUQAL(buf.writeableBytes(), blockSize - 400);

    buf.append(string(1000, 'z'));
    CHECK_EUQAL(buf.readableBytes(), 1350);

    buf.retrieve();
    CHECK_EUQAL(buf.readableBytes(), 0);
    CHECK_EUQAL(buf.writeableBytes(), 2048);
}

void testBufferShrink() {
  EventLoop loop;
  Buffer buf(&loop);
  buf.append(string(2000, 'y'));
  CHECK_EUQAL(buf.readableBytes(), 2000);
  CHECK_EUQAL(buf.writeableBytes(), 48);

  buf.retrieveBytes(1500);
  CHECK_EUQAL(buf.readableBytes(), 500);
  CHECK_EUQAL(buf.writeableBytes(), 1072);

  buf.shrink(0);
  CHECK_EUQAL(buf.readableBytes(), 500);
  CHECK_EUQAL(buf.writeableBytes(), 1072);

  size_t len = buf.readableBytes();
  char* msg = reinterpret_cast<char*>(malloc(sizeof(char) * len));
  buf.copyToUser(msg, len);
  const string str3(msg, len);
  CHECK_EUQAL(str3, string(500, 'y'));
}

int main() {
    printf("test begin... \n");
    testBufferAppendRetrieve();
    testBufferGrow();
    testBufferShrink();
    printf("test end , test case : %d, right case %d\n", testCase, rightCase);
}