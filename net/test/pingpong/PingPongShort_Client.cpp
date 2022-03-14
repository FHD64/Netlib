#include "TcpClient.h"
#include "Logger.h"
#include "Thread.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "InetAddress.h"
#include "Buffer.h"
#include "TcpConnection.h"
#include "Callback.h"
#include <utility>

#include <stdio.h>
#include <unistd.h>
#include <atomic>
using namespace netlib;
using namespace netlib::net;
using namespace std::placeholders;

class Client;
int testnum = 0;
std::atomic<int64_t> totalBytesRead(0);
std::atomic<int64_t> totalMessagesRead(0);
class Session : noncopyable {
 public:
  Session(EventLoop* loop,
          const InetAddress& serverAddr,
          const string& name,
          Client* owner)
    : client_(loop, serverAddr, name),
      owner_(owner),
      bytesRead_(0),
      bytesWritten_(0),
      messagesRead_(0) {
      client_.setConnectionCallback(std::bind(&Session::onConnection, this, _1));
      client_.setMessageCallback(std::bind(&Session::onMessage, this, _1, _2));
  }

  void start() {
    client_.connect();
  }

  void stop() {
    client_.disconnect();
  }

  int64_t bytesRead() const {
     return bytesRead_;
  }

  int64_t messagesRead() const {
     return messagesRead_;
  }

 private:

  void onConnection(const TcpConnectionPtr& conn);

  void onMessage(const TcpConnectionPtr& conn, Buffer* buf) {
    ++messagesRead_;
    bytesRead_ += buf->readableBytes();
    bytesWritten_ += buf->readableBytes();
    conn->send(buf);
  }

  TcpClient client_;
  Client* owner_;
  int64_t bytesRead_;
  int64_t bytesWritten_;
  int64_t messagesRead_;
};

class Client : noncopyable
{
 public:
  Client(EventLoop* loop,
         const InetAddress& serverAddr,
         int blockSize,
         int sessionCount,
         int timeout,
         int threadCount)
    : loop_(loop),
      threadPool_(loop, "pingpong-client"),
      sessionCount_(sessionCount),
      timeout_(timeout),
      numConnected_(0)
  {
    loop->runAfter(timeout, std::bind(&Client::handleTimeout, this));
    if (threadCount > 1)
    {
      threadPool_.setThreadNum(threadCount);
    }
    threadPool_.start();

    for (int i = 0; i < blockSize; ++i)
    {
      message_.push_back(static_cast<char>(i % 128));
    }

    for (int i = 0; i < sessionCount; ++i) {
      char buf[32];
      snprintf(buf, sizeof buf, "C%05d", i);
      Session* session = new Session(threadPool_.getLoop(), serverAddr, buf, this);
      session->start();
      sessions_.emplace_back(session); 
    }
  }

  const string& message() const
  {
    return message_;
  }

  void onConnect()
  {
    if (++numConnected_ == sessionCount_)
    {
      LOG_WARN << "all connected";
    }
  }

  void onDisconnect(const TcpConnectionPtr& conn)
  {
    if (--numConnected_ == 0)
    {
      LOG_WARN << "all disconnected";

      for (const auto& session : sessions_)
      {
        totalBytesRead += session->bytesRead();
        totalMessagesRead += session->messagesRead();
      }
      conn->getLoop()->queueInLoop(std::bind(&Client::quit, this));
    }
  }

 private:

  void quit()
  {
    loop_->queueInLoop(std::bind(&EventLoop::quit, loop_));
  }

  void handleTimeout()
  {
    LOG_WARN << "stop";
    for (auto& session : sessions_)
    {
      session->stop();
    }
  }

  EventLoop* loop_;
  EventLoopThreadPool threadPool_;
  int sessionCount_;
  int timeout_;
  std::vector<std::unique_ptr<Session>> sessions_;
  string message_;
  std::atomic<int> numConnected_;
};

void Session::onConnection(const TcpConnectionPtr& conn)
{
  if (conn->connected())
  {
    conn->setTcpNoDelay(true);
    conn->send(owner_->message());
    owner_->onConnect();
  }
  else
  {
    owner_->onDisconnect(conn);
  }
}

int main(int argc, char* argv[])
{
    LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid();
    Logger::setLogLevel(Logger::WARN);

    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    int threadCount = atoi(argv[2]);
    int blockSize = atoi(argv[3]);
    int sessionCount = atoi(argv[4]);
    int timeout = atoi(argv[5]);
    int testtime = atoi(argv[6]);
    testnum = testtime / timeout;
    EventLoop loop;
    InetAddress serverAddr("0.0.0.0", port);
    Timestamp begin(Timestamp::now());
    while(testnum) {
        Client client(&loop, serverAddr, blockSize, sessionCount, timeout, threadCount);
        loop.loop();
        testnum--;
    }
      LOG_WARN << totalBytesRead.load() << " total bytes read";
      LOG_WARN << totalMessagesRead.load() << " total messages read";
      LOG_WARN << static_cast<double>(totalBytesRead.load()) / static_cast<double>(totalMessagesRead.load())
               << " average message size";
      LOG_WARN << static_cast<double>(totalBytesRead.load()) / 
                (static_cast<double>(Timestamp::now().getUsSinceEpoch() - begin.getUsSinceEpoch()) / Timestamp::kUsPerSecond * 1024 * 1024)
               << " MiB/s throughput";
}