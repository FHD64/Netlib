#ifndef __NET_TCPCONNECTION__
#define __NET_TCPCONNECTION__

#include <boost/any.hpp>
#include <memory>
#include <netinet/tcp.h>
#include <string>
#include "InetAddress.h"
#include "Socket.h"
#include "Channel.h"
#include "Buffer.h"
#include "Callback.h"
#include "Timestamp.h"

namespace netlib {
namespace net {

class EventLoop;
using std::string;

void defaultConnectionCallback(const TcpConnectionPtr& conn);
void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer* buffer);

class TcpConnection : noncopyable,
                    public std::enable_shared_from_this<TcpConnection> {
 public:
  TcpConnection(EventLoop* loop, const string& name, int sock, const InetAddress& localAddr, const InetAddress& peerAddr);
  ~TcpConnection();

  //获取当前状态信息
  EventLoop* getLoop() const {
      return loop_;
  }
  const string& name() const {
      return name_;
  }
  const InetAddress& localAddress() const {
      return localAddr_;
  }
  const InetAddress& peerAddress() const {
      return peerAddr_;
  }
  bool connected() const {
      return state_ == kConnected;
  }
  bool disconnected() const {
      return state_ == kDisconnected;
  }
  const boost::any& getContext() const {
      return context_;
  }
  boost::any* getMutableContext() {
      return &context_;
  }

  //设置回调函数集
  void setContext(const boost::any& context) {
      context_ = context;
  }
  void setConnectionCallback(const ConnectionCallback& cb) {
      connectionCallback_ = cb;
  }
  void setMessageCallback(const MessageCallback& cb) {
      messageCallback_ = cb;
  }
  void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
      writeCompleteCallback_ = cb;
  }
  void setHighWaterMarkCallback(const HighWaterMarkCallback& cb) {
      highWaterMarkCallback_ = cb;
  }
  void setCloseCallback(const CloseCallback& cb) {
      closeCallback_ = cb;
  }
  void setTcpNoDelay(bool on) {
      socket_->setTcpNoDelay(on);
  }

  //初始化连接
  void init(EventLoop* loop, string& name, int sock, InetAddress& localAddr, InetAddress& peerAddr);

  //返回缓冲区内容
  Buffer* inputBuffer() {
      return &inputBuffer_;
  }
  Buffer* outputBuffer() {
      return &outputBuffer_;
  }
  
  bool getTcpInfo(struct tcp_info*) const;
  void send(const void* message, int len);
  void send(const StringPiece& message);
  void send(Buffer* message);
  void forceClose();
  void startRead();
  void connectDestroyed();
  void connectEstablished();
  void shutdown();
  void stopRead();
  friend class TcpConnectionPool;
 private:
  enum stateE {
      kDisconnected,
      kDisconnecting,
      kConnected,
      kConnecting
  };
  string name_;
  EventLoop* loop_;
  stateE state_;
  bool reading_;
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;
  InetAddress localAddr_;
  InetAddress peerAddr_;
  MessageCallback messageCallback_;
  ConnectionCallback connectionCallback_;
  CloseCallback closeCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  HighWaterMarkCallback highWaterMarkCallback_;
  size_t highWaterMark_;
  Buffer inputBuffer_;
  Buffer outputBuffer_;
  boost::any context_;

  void forceCloseWithDelay(double seconds);
  void shutdownInLoop();
  void forceCloseInLoop();
  void handleRead();
  void handleWrite();
  void handleClose();
  void handleError();
  void sendInLoop(const StringPiece& message);
  void sendInLoop(const void* message, size_t len);
  void startReadInLoop();
  void stopReadInLoop();
  const char* stateToString() const;
};
}
}
#endif