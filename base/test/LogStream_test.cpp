#include "LogStream.h"

#include <limits>
#include <stdint.h>
#include <string>

using std::string;
int testCase = 0;
int rightCase = 0;
#define CHECK_EUQAL(a, b) \
    do { \
        testCase++; \
        if(a == b) { \
            rightCase++; \
            printf("right case : func %s, Line %d\n", __func__, __LINE__); \
        } else { \
            printf("wrong case : func %s, Line %d\n", __func__, __LINE__); \
        } \
    } while(0)

void testBoolean() {
    printf("Test Boolean begin...\n");
    netlib::LogStream os;
    const netlib::LogStream::Buffer& buf = os.buffer();
    CHECK_EUQAL(buf.toString(), string(""));
    os << true;
    CHECK_EUQAL(buf.toString(), string("1"));
    os << '\n';
    CHECK_EUQAL(buf.toString(), string("1\n"));
    os << false;
    CHECK_EUQAL(buf.toString(), string("1\n0"));
}

void testInt() {
    printf("Test Int begin...\n");
    netlib::LogStream os;
    const netlib::LogStream::Buffer& buf = os.buffer();
    CHECK_EUQAL(buf.toString(), string(""));
    os << 1;
    CHECK_EUQAL(buf.toString(), string("1"));
    os << 0;
    CHECK_EUQAL(buf.toString(), string("10"));
    os << -1;
    CHECK_EUQAL(buf.toString(), string("10-1"));
    os.resetBuffer();

    os << 0 << " " << 123 << 'x' << 0x64;
    CHECK_EUQAL(buf.toString(), string("0 123x100"));
}

void testIntLimit() {
    printf("Test Int Limit begin...\n");
    netlib::LogStream os;
    const netlib::LogStream::Buffer& buf = os.buffer();
    os << -2147483647;
    CHECK_EUQAL(buf.toString(), string("-2147483647"));
    os << static_cast<int>(-2147483647 - 1);
    CHECK_EUQAL(buf.toString(), string("-2147483647-2147483648"));
    os << ' ';
    os << 2147483647;
    CHECK_EUQAL(buf.toString(), string("-2147483647-2147483648 2147483647"));
    os.resetBuffer();

    os << std::numeric_limits<int16_t>::min();
    CHECK_EUQAL(buf.toString(), string("-32768"));
    os.resetBuffer();

    os << std::numeric_limits<int16_t>::max();
    CHECK_EUQAL(buf.toString(), string("32767"));
    os.resetBuffer();

    os << std::numeric_limits<uint16_t>::min();
    CHECK_EUQAL(buf.toString(), string("0"));
    os.resetBuffer();

    os << std::numeric_limits<uint16_t>::max();
    CHECK_EUQAL(buf.toString(), string("65535"));
    os.resetBuffer();

    os << std::numeric_limits<int32_t>::min();
    CHECK_EUQAL(buf.toString(), string("-2147483648"));
    os.resetBuffer();

    os << std::numeric_limits<int32_t>::max();
    CHECK_EUQAL(buf.toString(), string("2147483647"));
    os.resetBuffer();

    os << std::numeric_limits<uint32_t>::min();
    CHECK_EUQAL(buf.toString(), string("0"));
    os.resetBuffer();

    os << std::numeric_limits<uint32_t>::max();
    CHECK_EUQAL(buf.toString(), string("4294967295"));
    os.resetBuffer();

    os << std::numeric_limits<int64_t>::min();
    CHECK_EUQAL(buf.toString(), string("-9223372036854775808"));
    os.resetBuffer();

    os << std::numeric_limits<int64_t>::max();
    CHECK_EUQAL(buf.toString(), string("9223372036854775807"));
    os.resetBuffer();

    os << std::numeric_limits<uint64_t>::min();
    CHECK_EUQAL(buf.toString(), string("0"));
    os.resetBuffer();

    os << std::numeric_limits<uint64_t>::max();
    CHECK_EUQAL(buf.toString(), string("18446744073709551615"));
    os.resetBuffer();

    int16_t a = 0;
    int32_t b = 0;
    int64_t c = 0;
    os << a;
    os << b;
    os << c;
    CHECK_EUQAL(buf.toString(), string("000"));
}

void testFloat() {
    printf("Test Int Limit begin...\n");
    netlib::LogStream os;
    const netlib::LogStream::Buffer& buf = os.buffer();

    os << 0.0;
    CHECK_EUQAL(buf.toString(), string("0"));
    os.resetBuffer();

    os << 1.0;
    CHECK_EUQAL(buf.toString(), string("1"));
    os.resetBuffer();

    os << 0.1;
    CHECK_EUQAL(buf.toString(), string("0.1"));
    os.resetBuffer();

    os << 0.05;
    CHECK_EUQAL(buf.toString(), string("0.05"));
    os.resetBuffer();

    os << 0.15;
    CHECK_EUQAL(buf.toString(), string("0.15"));
    os.resetBuffer();

    double a = 0.1;
    os << a;
    CHECK_EUQAL(buf.toString(), string("0.1"));
    os.resetBuffer();

    double b = 0.05;
    os << b;
    CHECK_EUQAL(buf.toString(), string("0.05"));
    os.resetBuffer();

    double c = 0.15;
    os << c;
    CHECK_EUQAL(buf.toString(), string("0.15"));
    os.resetBuffer();

    os << a+b;
    CHECK_EUQAL(buf.toString(), string("0.15"));
    os.resetBuffer();

    os << 1.23456789;
    CHECK_EUQAL(buf.toString(), string("1.23456789"));
    os.resetBuffer();

    os << 1.234567;
    CHECK_EUQAL(buf.toString(), string("1.234567"));
    os.resetBuffer();

    os << -123.456;
    CHECK_EUQAL(buf.toString(), string("-123.456"));
    os.resetBuffer();
}

void testFmt() {
    printf("Test Fmt begin...\n");
    netlib::LogStream os;
    const netlib::LogStream::Buffer& buf = os.buffer();

    os << netlib::Fmt("%4d", 1);
    CHECK_EUQAL(buf.toString(), string("   1"));
    os.resetBuffer();

    os << netlib::Fmt("%4.2f", 1.2);
    CHECK_EUQAL(buf.toString(), string("1.20"));
    os.resetBuffer();

    os << netlib::Fmt("%4.2f", 1.2) << netlib::Fmt("%4d", 43);
    CHECK_EUQAL(buf.toString(), string("1.20  43"));
    os.resetBuffer();
}

void testLong() {
    netlib::LogStream os;
    const netlib::LogStream::Buffer& buf = os.buffer();
    for (int i = 0; i < 399; ++i) {
      os << "123456789 ";
      CHECK_EUQAL(buf.length(), 10*(i+1));
      CHECK_EUQAL(buf.avail(), 4000 - 10*(i+1));
    }

    os << "abcdefghi ";
    CHECK_EUQAL(buf.length(), 3990);
    CHECK_EUQAL(buf.avail(), 10);

    os << "abcdefghi";
    CHECK_EUQAL(buf.length(), 3999);
    CHECK_EUQAL(buf.avail(), 1);
}

int main() {
    testBoolean();
    testInt();
    testIntLimit();
    testLong();
    testFloat();
    testFmt();

    printf("test case : %d, right case %d \n", testCase, rightCase);
}
