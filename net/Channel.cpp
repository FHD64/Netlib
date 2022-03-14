#include "Channel.h"
#include "Logger.h"
#include "EventLoop.h"
#include <sys/epoll.h>
#include <poll.h>

using namespace netlib;
using namespace netlib::net;

Channel::Channel(EventLoop* loop, int fd)
        : tied_(false),
          fd_(fd),
          events_(0),
          revents_(0),
          loghup_(true),
          state_(-1),
          loop_(loop){
}

Channel::~Channel() {
    if(loop_->isInLoopThread() && loop_->hasChannel(this)) {
        remove();
    }
}

void Channel::handleEvent() {
    //判断当前loop是否存在
    std::shared_ptr<void> guard;
    if(tied_ ) {
        guard = tie_.lock();
        if(!guard)
            return;
    }

    //若当前对端socket关闭，执行关闭动作
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
        if (loghup_)
            LOG_WARN << "fd = " << fd_ << " Channel::handle_event() EPOLLHUP";

        if (closeCallback_) 
            closeCallback_();
    }

    if (revents_ & (POLLERR | POLLNVAL)) {
        if (errorCallback_) 
            errorCallback_();
    }
    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
        if (readCallback_) 
            readCallback_();
    }
    if (revents_ & POLLOUT) {
        if (writeCallback_)
            writeCallback_();
    }
}

void Channel::update() {
      loop_->updateChannel(this);
}

void Channel::remove() {
      loop_->removeChannel(this);
}

void Channel::reset() {
    if(loop_->isInLoopThread() && loop_->hasChannel(this)) {
        remove();
    }
    tied_ = false;
    events_ = 0;
    revents_ = 0;
    loghup_ = true;
    state_ = -1;
}