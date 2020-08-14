#include "Channel.h"
#include "EventLoop.h"
#include "../base/Logging.h"

#include <sstream>

#include <poll.h>

using namespace muduo;
using namespace muduo::net;
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop,int fdArg)
    : loop_(loop),
    fd_(fdArg),
    events_(0),
    revents_(0),
    index_(-1)
{
}
/*
Channel::update() 会调用EventLoop::updateChannel(),后者转而调用Poller::updateChannel()
*/
void Channel::update()
{
    loop_->updateChannel(this);
}
/*
是Channel的核心，它由EventLoop::loop()调用，它的功能是根据revents_的值分别调用不同的用户回调
*/
void Channel::handleEvent(){
    if (revents_ & POLLNVAL) {
        LOG_WARN << "Channel::handle_event() POLLNVAL";
    }

    if (revents_ & (POLLERR | POLLNVAL)) {
        if (errorCallback_) errorCallback_();
    }
    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
        if (readCallback_) readCallback_();
    }
    if (revents_ & POLLOUT) {
        if (writeCallback_) writeCallback_();
    }
}