#ifndef __NET_EVENTLOOP__
#define __NET_EVENTLOOP__

#include <atomic>
#include <memory>
#include <functional>
#include <vector>
#include <boost/any.hpp>
#include "Mutex.h"
#include "BufferPool.h"
#include "CurrentThread.h"
#include "Timestamp.h"
#include "TimerId.h"
#include "TimerQueue.h"
#include "noncopyable.h"
#include "Poller.h"
#include "Callback.h"
#include "TcpConnectionPool.h"

namespace netlib {

namespace net {

class EventLoop : noncopyable {
 public:
  typedef std::function<void()> Functor;

  EventLoop();
  ~EventLoop();

  //loop循环函数
  void loop();

  //退出loop循环
  void quit();

  //在loop中调用回调。
  //该函数可由loop绑定线程之外的其他线程调用
  //若为loop绑定线程调用则直接运行
  //否则enqueue至队列，等待loop处理
  void wakeup();
  void runInLoop(Functor cb);
  void queueInLoop(Functor cb);
  size_t pendQueueSize() const;

  //定时器相关
  //1.一次性定时器，可指定运行时间或x时间后运行
  //2.通用定时器，指定间隔定时时间
  TimerId runAt(Timestamp time, TimerCallback cb);
  TimerId runAfter(double delay, TimerCallback cb);
  TimerId runEvery(double interval, TimerCallback cb);
  void cancelTimer(TimerId timerid);

  //删/改 channel
  void updateChannel(Channel* channel);
  void removeChannel(Channel* channel);
  bool hasChannel(Channel* channel);

  //loop线程相关
  bool isInLoopThread() const {
      return threadid_ == CurrentThread::tid();
  }
  void setContext(const boost::any& context) {
      context_ = context;
  }
  const boost::any& getContext() const {
      return context_;
  }
  boost::any* getMutableContext() {
      return &context_;
  }
  Connections* connections() {
      return &connections_;
  }

  BufferBlock* allocate() {
      if(isInLoopThread()) {
          return bufferPool_->allocate();
      }
      return NULL;
  }
  void free(BufferBlock* block) {
      if(!block) {
          return;
      }
      bufferPool_->free(block);
  }
  TcpConnection* alloConnection(EventLoop* loop, string& name, int sock, InetAddress& localAddr, InetAddress& peerAddr) {
      if(isInLoopThread()) {
          return connectionPool_->allocate(loop, name, sock, localAddr, peerAddr);
      }
      return NULL;
  }
  void freeConnection(TcpConnection* conn) {
      if(isInLoopThread()) {
          connectionPool_->free(conn);
          return;
      }

      queueInLoop(std::bind(&EventLoop::freeConnectionInLoop, this, conn));
  }

  static EventLoop* getEventLoopOfCurrentThread();

 private:
  void handleRead();
  void doPendingFunctors();
  void freeConnectionInLoop(TcpConnection* conn) {
      connectionPool_->free(conn);
  }
  
  typedef std::vector<Channel*> ChannelList;
  std::atomic<bool> looping_;
  std::atomic<bool> callingpendingfunc_;
  const pid_t threadid_;
  Timestamp epollreturntime_;
  std::unique_ptr<Poller> epoller_;
  std::unique_ptr<TimerQueue> timerqueue_;
  int wakeupfd_;
  std::unique_ptr<Channel> wakeupchannel_;
  std::unique_ptr<BufferPool> bufferPool_;
  
  boost::any context_;
  ChannelList activechannels_;
  mutable Mutex mutex_;
  std::vector<Functor> pendingfunctors_;
  Connections connections_;
  std::unique_ptr<TcpConnectionPool> connectionPool_;
};

}
}
#endif