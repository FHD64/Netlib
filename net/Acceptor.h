#ifndef __NET_ACCEPTOR__
#define __NET_ACCEPTOR__

#include "Channel.h"
#include "Socket.h"
#include "InetAddress.h"

namespace netlib {

namespace net {

class EventLoop;
class Acceptor : noncopyable{
 public:
  typedef std::function<void(int fd, const InetAddress&)> NewConnectionCallback;

  Acceptor(EventLoop* loop, const InetAddress& listenaddr, bool reuseport = true);
  ~Acceptor();

  void setNewConnectionCallback(const NewConnectionCallback& cb) {
      newconnectioncallback_ = cb;
  }

  void listen();
  bool listening() const {
      return listening_;
  }

 private:
  void handleRead();

  EventLoop* loop_;
  Socket acceptsock_;
  Channel acceptchannel_;
  NewConnectionCallback newconnectioncallback_;
  bool listening_;
  int idlefd_;
};
}
}
#endif