#ifndef __NET_EPOLLER__
#define __NET_EPOLLER__

#include <vector>
#include <map>
#include "noncopyable.h"
#include "Timestamp.h"

struct epoll_event;
//EPOLLER采用epoll多路复用，是多路选择器的抽象实现
//Epoller负责管理(添加/管理/删除)Channel，根据事件实时管理Channel下event
namespace netlib {

namespace net{

class EventLoop;
class Channel;

class Poller : noncopyable {
 public:
  typedef std::vector<Channel*> ChannelList;

  Poller(EventLoop* el);
  ~Poller();
  
  //channel管理功能
  Timestamp epoll(int timeout, ChannelList* activeChannels);
  void updateChannel(Channel* channel);
  void removeChannel(Channel* channel);

  bool hasChannel(Channel* channel) const;

 private:
  static const int knew = -1;
  static const int kadded = 0;
  static const int kdeleted = 1;
  void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
  void update(int operation, Channel* channel);
  std::map<int, Channel*> channels_;
  //TODO: 移植性问题？是否改成数组
  std::vector<struct epoll_event> events_;
  int epollfd_;
  EventLoop* ownerloop_;
};
}
}
#endif