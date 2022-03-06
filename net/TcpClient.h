#ifndef __NET_TCPCLIENT__
#define __NET_TCPCLIENT__

#include <atomic>
#include <string>
#include "Callback.h"
#include "noncopyable.h"
#include "InetAddress.h"
#include "Mutex.h"


namespace netlib {

namespace net {

class Connector;
class EventLoop;
using std::string;

class TcpClient : noncopyable {
 public:
  TcpClient(EventLoop* loop, const InetAddress& serverAddr, const string& name);
  ~TcpClient();

  void connect();
  void disconnect();
  void stop();

  TcpConnectionPtr connection() const {
      MutexLock lock(&mutex_);
      return connection_;
  }
  EventLoop* getLoop() const {
      return loop_;
  }
  bool retry() const {
      return retry_;
  }
  string name() const {
      return name_;
  }

  void setConnectionCallback(ConnectionCallback cb) {
      connectionCallback_ = std::move(cb);
  }
  void setMessageCallback(MessageCallback cb) {
      messageCallback_ = std::move(cb);
  }
  void setWriteCompleteCallback(WriteCompleteCallback cb) {
      writeCompleteCallback_ = std::move(cb);
  }

 private:
  void removeConnection(const TcpConnectionPtr& conn);
  void newConnection(int sockfd);
  EventLoop* loop_;
  ConnectorPtr connector_;
  const std::string name_;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  std::atomic<bool> retry_;
  std::atomic<bool> connect_;
  int nextid_;
  mutable Mutex mutex_;
  TcpConnectionPtr connection_;
};
}
}
#endif