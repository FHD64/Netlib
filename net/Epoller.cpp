#define NET_EPOLLER

#ifdef NET_EPOLLER
#include "Poller.h"
#include "Logger.h"
#include "Channel.h"
#include <sys/epoll.h>

using namespace netlib;
using namespace netlib::net;

Poller::Poller(EventLoop* loop)
    : events_(16),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      ownerloop_(loop){
}

Poller::~Poller() {
    ::close(epollfd_);
}

void Poller::update(int op, Channel* channel) {
    struct epoll_event event;
    ::memset(&event, 0, sizeof event);
    event.events = channel->events();
    event.data.ptr = channel;

    int fd = channel->fd();
    if(::epoll_ctl(epollfd_, op, fd, &event) < 0)
        LOG_SYSERR << "epoll_ctl op = " << op << "fd = " << fd;
}

void Poller::fillActiveChannels(int numevent, ChannelList* activechannel) const {
    for(int i = 0; i < numevent; i++) {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->setRevents(events_[i].events);
        activechannel->push_back(channel);
    }
}

Timestamp Poller::epoll(int timeout, ChannelList* activechannel) {
    int numevent = ::epoll_wait(epollfd_, &(*(events_.begin())), static_cast<int>(events_.size()), timeout);

   if((numevent < 0) && (errno != EINTR)) {
       LOG_SYSERR << "Poller::epoll()";
       return Timestamp::now();
    }

    fillActiveChannels(numevent, activechannel);
    if(static_cast<size_t>(numevent) == events_.size())
        events_.resize(events_.size() * 2);

    return Timestamp::now();
}

void Poller::updateChannel(Channel* channel) {
    const int state = channel->state();
    int fd = channel->fd();
    //若当前channel已从epoller中删除或为新channel，操作码选择EPOLL_CTL_ADD
    if((state == knew) || (state == kdeleted)) {
        if(state == knew)
            channels_[fd] = channel;
        
        channel->setState(kadded);
        update(EPOLL_CTL_ADD, channel);
    } else {
        if(channel->isNoneEvent()) {
            update(EPOLL_CTL_DEL, channel);
            channel->setState(kdeleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void Poller::removeChannel(Channel* channel) {
    int state = channel->state();
    int fd = channel->fd();
    channels_.erase(fd);

    if(state == kadded) 
        update(EPOLL_CTL_DEL, channel);
    
    channel->setState(knew);
}

bool Poller::hasChannel(Channel* channel) const {
    std::map<int, Channel*>::const_iterator cit = channels_.find(channel->fd());
    return ((cit != channels_.end()) && (cit->second == channel));
}
#endif