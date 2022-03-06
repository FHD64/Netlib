#ifndef __NET_TCPSERVER__
#define __NET_TCPSERVER__

#include "noncopyable.h"
#include "Callback.h"
#include "InetAddress.h"
#include "EventLoopThreadPool.h"
#include <string>
#include <map>
#include <atomic>

namespace netlib {

namespace net {

class Acceptor;
class EventLoop;
class EventLoopThreadPool;
using std::string;

class TcpServer : noncopyable {

 public:
  typedef std::function<void (EventLoop*)> ThreadInitCallback;

  TcpServer(EventLoop *loop, const InetAddress& listenAddr, const string& name, bool reusePort = false);
  ~TcpServer();

  const string& ipPort() const {
      return ipPort_;
  }
  const string& name() const {
      return name_;
  }
  EventLoop* getLoop() const {
      return loop_;
  }
  std::shared_ptr<EventLoopThreadPool> threadPool() {
      return threadpool_;
  }

  void setThreadInitCallback(const ThreadInitCallback &cb) {
      threadInitCallback_ = cb;
  }
  void setConnectionCallback(const ConnectionCallback &cb) {
      connectionCallback_ = cb;
  }
  void setMessageCallback(const MessageCallback &cb) {
      messageCallback_ = cb;
  }
  void setWriteCompleteCallback(const WriteCompleteCallback &cb) {
      writeCompleteCallback_ = cb;
  }
  void setThreadNum(int num) {
      threadpool_->setThreadNum(num);
  }
  
  void start();
  void removeConnection(const TcpConnectionPtr& conn);
 private:
  void newConnection(int sockfd, const InetAddress& peeraddr);
  void removeConnectionInLoop(const TcpConnectionPtr& conn);
  typedef std::map<string, TcpConnectionPtr> Connections;
  EventLoop *loop_;
  const string ipPort_;
  const string name_;
  std::unique_ptr<Acceptor> acceptor_;
  std::shared_ptr<EventLoopThreadPool> threadpool_;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  ThreadInitCallback threadInitCallback_;
  std::atomic<bool> started_;
  int nextid_;
  Connections connections_;
};
}
}
#endif