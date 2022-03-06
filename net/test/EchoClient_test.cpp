#include "TcpClient.h"
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
class EchoClient;
std::vector<std::unique_ptr<EchoClient>> clients;

class EchoClient : noncopyable {
 public:
  EchoClient(EventLoop* loop, const InetAddress& listenAddr, const string& id)
    : loop_(loop),
      client_(loop, listenAddr, "EchoClient"+id) {
          client_.setConnectionCallback(std::bind(&EchoClient::onConnection, this, _1));
          client_.setMessageCallback(std::bind(&EchoClient::onMessage, this, _1, _2));
  }

  void connect() {
    client_.connect();
  }

 private:
  void onConnection(const TcpConnectionPtr& conn) {
      LOG_TRACE << conn->localAddress().toIpPort() << " -> "
                << conn->peerAddress().toIpPort() << " is "
                << (conn->connected() ? "UP" : "DOWN");

      if (conn->connected()) {
          LOG_INFO << "*** connected ";
      }
      conn->send("world\n");
  }

  void onMessage(const TcpConnectionPtr& conn, Buffer* buf) {
      string msg(buf->retrieveAsString());
      LOG_TRACE << conn->name() << " recv " << msg.size() << " bytes at " << msg;
      if (msg == "quit\n") {
          conn->send("bye\n");
          conn->shutdown();
      } else if (msg == "shutdown\n") {
          loop_->quit();
      } else {
          conn->send(msg);
      }
  }

  EventLoop* loop_;
  TcpClient client_;
};

int main(int argc, char* argv[]) {
    LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid();

    EventLoop loop;
    bool ipv6 = false;
    InetAddress serverAddr("0.0.0.0", 2000, ipv6);

    int clientsNum = 1;
    clients.reserve(clientsNum);
    for (int i = 0; i < clientsNum; ++i) {
        char buf[32];
        snprintf(buf, sizeof buf, "%d", i+1);
        clients.emplace_back(new EchoClient(&loop, serverAddr, buf));
        clients[i]->connect();
    }
    
    loop.loop();
}
