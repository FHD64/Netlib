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
#include <iostream>

using namespace netlib;
using namespace netlib::net;
using namespace std::placeholders;

using std::string;

int numThreads = 0;

class EchoServer {
 public:
  EchoServer(EventLoop* loop, const InetAddress& listenAddr)
    : loop_(loop),
      server_(loop, listenAddr, "EchoServer") {
    server_.setConnectionCallback(
        std::bind(&EchoServer::onConnection, this, _1));
    server_.setMessageCallback(
        std::bind(&EchoServer::onMessage, this, _1, _2));
    server_.setThreadNum(numThreads);
  }

  void start() {
    server_.start();
  }

 private:
  void onConnection(const TcpConnectionPtr& conn) {
      LOG_TRACE << conn->peerAddress().toIpPort() << " -> "
                << conn->localAddress().toIpPort() << " is "
                << (conn->connected() ? "UP" : "DOWN");

      conn->send("hello\n");
  }

  void onMessage(const TcpConnectionPtr& conn, Buffer* buf) {
    string msg(buf->retrieveAsString());
    LOG_TRACE << conn->name() << " recv " << msg.size() << " bytes at " << msg;
    if (msg == "exit\n") {
        conn->send("bye\n");
        conn->shutdown();
    }
    if (msg == "quit\n") {
      loop_->quit();
    }
    conn->send(msg);
  }

  EventLoop* loop_;
  TcpServer server_;
};

int main(int argc, char* argv[]) {
    LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid();
    LOG_INFO << "sizeof TcpConnection = " << sizeof(TcpConnection);

    bool ipv6 = false;
    EventLoop loop;
    InetAddress listenAddr(2000, false, ipv6);
    EchoServer server(&loop, listenAddr);
    server.start();
    loop.loop();
}
