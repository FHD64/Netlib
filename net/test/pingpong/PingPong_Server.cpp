#include "TcpServer.h"
#include "Logger.h"
#include "Thread.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "TcpConnection.h"
#include "Callback.h"

#include <utility>
#include <stdio.h>
#include <unistd.h>
#include <atomic>

using namespace netlib;
using namespace netlib::net;

void onConnection(const TcpConnectionPtr& conn) {
    if (conn->connected()) {
        conn->setTcpNoDelay(true);
    }
}

void onMessage(const TcpConnectionPtr& conn, Buffer* buf) {
    conn->send(buf);
}

int main(int argc, char* argv[]) {

    LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid();
    Logger::setLogLevel(Logger::WARN);

    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    InetAddress listenAddr("0.0.0.0", port);
    int threadCount = atoi(argv[2]);

    EventLoop loop;

    TcpServer server(&loop, listenAddr, "PingPong");

    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);

    if (threadCount > 1) {
      server.setThreadNum(threadCount);
    }

    server.start();
    loop.loop();
}
