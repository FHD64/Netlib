#include "Buffer.h"
#include <string>
#include <iostream>
using std::string;
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
    Buffer buf;
    CHECK_EUQAL(buf.readableBytes(), 0);
    CHECK_EUQAL(buf.writeableBytes(), Buffer::kinitsize);
    CHECK_EUQAL(buf.prependBytes(), Buffer::kprepend);

    const string str(200, 'x');
    buf.append(str);
    CHECK_EUQAL(buf.readableBytes(), str.size());
    CHECK_EUQAL(buf.writeableBytes(), Buffer::kinitsize - str.size());
    CHECK_EUQAL(buf.prependBytes(), Buffer::kprepend);

    const string str2 =  buf.retrieveBytesAsString(50);
    CHECK_EUQAL(str2.size(), 50);
    CHECK_EUQAL(buf.readableBytes(), str.size() - str2.size());
    CHECK_EUQAL(buf.writeableBytes(), Buffer::kinitsize - str.size());
    CHECK_EUQAL(buf.prependBytes(), Buffer::kprepend + str2.size());
    CHECK_EUQAL(str2, string(50, 'x'));

    buf.append(str);
    CHECK_EUQAL(buf.readableBytes(), 2*str.size() - str2.size());
    CHECK_EUQAL(buf.writeableBytes(), Buffer::kinitsize - 2 * str.size());
    CHECK_EUQAL(buf.prependBytes(), Buffer::kprepend + str2.size());

    const string str3 =  buf.retrieveAsString();
    CHECK_EUQAL(str3.size(), 350);
    CHECK_EUQAL(buf.readableBytes(), 0);
    CHECK_EUQAL(buf.writeableBytes(), Buffer::kinitsize);
    CHECK_EUQAL(buf.prependBytes(), Buffer::kprepend);
    CHECK_EUQAL(str3, string(350, 'x'));
}

void testBufferGrow() {
    Buffer buf;
    buf.append(string(400, 'y'));
    CHECK_EUQAL(buf.readableBytes(), 400);
    CHECK_EUQAL(buf.writeableBytes(), Buffer::kinitsize-400);

    buf.retrieveBytes(50);
    CHECK_EUQAL(buf.readableBytes(), 350);
    CHECK_EUQAL(buf.writeableBytes(), Buffer::kinitsize-400);
    CHECK_EUQAL(buf.prependBytes(), Buffer::kprepend+50);

    buf.append(string(1000, 'z'));
    CHECK_EUQAL(buf.readableBytes(), 1350);
    CHECK_EUQAL(buf.prependBytes(), Buffer::kprepend);

    buf.retrieve();
    CHECK_EUQAL(buf.readableBytes(), 0);
    CHECK_EUQAL(buf.writeableBytes(), 1400);
    CHECK_EUQAL(buf.prependBytes(), Buffer::kprepend);
}

void testBufferShrink() {
  Buffer buf;
  buf.append(string(2000, 'y'));
  CHECK_EUQAL(buf.readableBytes(), 2000);
  CHECK_EUQAL(buf.writeableBytes(), 0);
  CHECK_EUQAL(buf.prependBytes(), Buffer::kprepend);

  buf.retrieveBytes(1500);
  CHECK_EUQAL(buf.readableBytes(), 500);
  CHECK_EUQAL(buf.writeableBytes(), 0);
  CHECK_EUQAL(buf.prependBytes(), Buffer::kprepend+1500);

  buf.shrink(0);
  CHECK_EUQAL(buf.readableBytes(), 500);
  CHECK_EUQAL(buf.writeableBytes(), Buffer::kinitsize-500);
  CHECK_EUQAL(buf.retrieveAsString(), string(500, 'y'));
  CHECK_EUQAL(buf.prependBytes(), Buffer::kprepend);
}

void testBufferPrepend() {
    Buffer buf;
    buf.append(string(200, 'y'));
    CHECK_EUQAL(buf.readableBytes(), 200);
    CHECK_EUQAL(buf.writeableBytes(), Buffer::kinitsize-200);
    CHECK_EUQAL(buf.prependBytes(), Buffer::kprepend);

    int x = 0;
    buf.setPrepend(&x, sizeof x);
    CHECK_EUQAL(buf.readableBytes(), 204);
    CHECK_EUQAL(buf.writeableBytes(), Buffer::kinitsize-200);
    CHECK_EUQAL(buf.prependBytes(), Buffer::kprepend - 4);
}

void output(Buffer&& buf, const void* inner) {
    Buffer newbuf(std::move(buf));
    CHECK_EUQAL(inner, newbuf.peek());
}

void testMove() {
    Buffer buf;
    buf.append("netlib", 6);
    const void* inner = buf.peek();
    output(std::move(buf), inner);
}

int main() {
    printf("test begin... \n");
    testBufferAppendRetrieve();
    testBufferGrow();
    testBufferShrink();
    testBufferPrepend();
    testMove();
    printf("test end , test case : %d, right case %d\n", testCase, rightCase);
}