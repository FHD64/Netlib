#include "Logger.h"
#include "Thread.h"

#include <stdio.h>
#include <unistd.h>
#include <vector>

using namespace netlib;
FILE* file = fopen("/home/f00581155/error", "rb+");
void ownOutput(const char* msg, int len) {
    fwrite(msg, 1, len, file);
}

void logInThread() {
    LOG_INFO << "logInThread";
    usleep(1000);
}

int main() {
    std::vector<std::unique_ptr<netlib::Thread>> Threads;

    for(int i = 0; i < 8; i++) {
        Threads.emplace_back(new netlib::Thread(&logInThread));
        Threads[i]->start();
    }

    for(int i = 0; i < 8; i++) {
        Threads[i]->join();
    }

    LOG_TRACE << "trace";
    LOG_DEBUG << "debug";
    LOG_INFO << "Hello";
    LOG_WARN << "World";
    LOG_ERROR << "Error";
    LOG_INFO << sizeof(netlib::Logger);
    LOG_INFO << sizeof(netlib::LogStream);
    LOG_INFO << sizeof(netlib::Fmt);
    LOG_INFO << sizeof(netlib::LogStream::Buffer);

    sleep(1);
    netlib::Logger::setOutput(ownOutput);
    LOG_TRACE << "trace";
    LOG_DEBUG << "debug";
    LOG_INFO << "Hello";
    LOG_WARN << "World";
    LOG_ERROR << "Error";
    sleep(1);
}