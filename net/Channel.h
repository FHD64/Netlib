#ifndef __NET_CHANNEL__
#define __NET_CHANNEL__

#include "noncopyable.h"
#include "Timestamp.h"
#include <functional>
#include <sys/epoll.h>
#include <memory>

namespace netlib {

namespace net {

class EventLoop;

class Channel : noncopyable {
 public:
  
  typedef std::function<void()> EventCallback;
  Channel(EventLoop *loop, int fd);
  ~Channel();
  
  //事件设置、处理
  void handleEvent();
  void setReadCallback(EventCallback cb) {
      readCallback_ = std::move(cb);
  }
  void setWriteCallback(EventCallback cb) {
      writeCallback_ = std::move(cb);
  }
  void setCloseCallback(EventCallback cb) {
      closeCallback_ = std::move(cb);
  }
  void setErrorCallback(EventCallback cb) {
      errorCallback_ = std::move(cb);
  }
 
  void tie(const std::shared_ptr<void>& obj) {
      tie_ = obj;
      tied_ = true;
  }
  int fd() const {
      return fd_;
  }
  int events() const {
      return events_;
  }
  void setRevents(int re) {
      revents_ = re;
  }
  bool isNoneEvent() const {
      return events_ == kNoneEvent;
  }
  void reset();
  void reset(EventLoop* loop, int fd) {
      loop_ = loop;
      fd_ = fd;
  }
  //使能读写事件
  void enableReading() {
      events_ |= kReadEvent;
      update();
  }
  void disableReading() {
      events_ &= ~kReadEvent;
      update();
  }
  void enableWriting() {
      events_ |= kWriteEvent;
      update();
  }
  void disableWriting() {
      events_ &= ~kWriteEvent;
      update();
  }
  void disableAll() {
      events_ = kNoneEvent;
      update();
  }
  bool isWriting() const {
      return events_ & kWriteEvent;
  }
  bool isReading() const {
      return events_ & kReadEvent;
  }

  void donotLogHup() {
      loghup_ = false;
  }
  EventLoop* ownerLoop() {
      return loop_;
  }

  //刷新/移除当前channel
  void update();
  void remove();

  //设置/返回Channel管理状态
  int state() const {
      return state_;
  }
  void setState(int state) {
      state_ = state;
  }
  
 private:
  static const int kNoneEvent = 0;
  static const int kReadEvent = EPOLLIN | EPOLLPRI;
  static const int kWriteEvent = EPOLLOUT;

  std::weak_ptr<void> tie_;
  bool tied_;

  int fd_;
  
  //当前事件
  int events_;
  //需响应事件
  int revents_;
  bool loghup_;
  //标识当前channel管理状态
  int state_;
  EventLoop* loop_;
  EventCallback readCallback_;
  EventCallback writeCallback_;
  EventCallback closeCallback_;
  EventCallback errorCallback_;
};

}
}
#endif