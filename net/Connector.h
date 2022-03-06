#ifndef __NET_CONNECTOR__
#define __NET_CONNECTOR__

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callback.h"
#include "Logger.h"

#include <atomic>
#include <memory>

namespace netlib {

namespace net {

class Channel;
class EventLoop;

class Connector : noncopyable, public std::enable_shared_from_this<Connector> {

 public:
  typedef std::function<void (int sockfd)> NewConnectionCallback;
  Connector(EventLoop* loop, const InetAddress& serveraddr);
  ~Connector();

  void setNewConnectionCallback(NewConnectionCallback cb) {
      newConnectionCallback_ = cb;
  }
  const InetAddress& serverAddress() const {
      return serverAddr_;
  }

  void start();
  void restart();
  void stop();

 private:
  enum States {
      kDisconnected,
      kConnecting,
      kConnected
  };

  static const int kMaxRetryMs = 30 * 1000;
  static const int kInitRetryMs = 500;
  EventLoop* loop_;
  InetAddress serverAddr_;
  std::atomic<bool> connect_;
  States state_;
  std::unique_ptr<Channel> channel_;
  NewConnectionCallback newConnectionCallback_;
  int retryMs_;
  
  void startInLoop();
  void stopInLoop();
  void connect();
  void connecting(int sock);
  void handleWrite();
  void handleError();
  void retry(int sock);
  int removeAndResetChannel();
  void resetChannel();

};
}
}
#endif