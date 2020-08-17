/*
用于每个socket连接的事件分发
*/
#ifndef MUDUO_NET_CHANNEL_H
#define MUDUO_NET_CHANNEL_H

#include "../base/noncopyable.h"
#include "../base/Timestamp.h"

#include <functional>
#include <memory>

namespace muduo
{
namespace net
{

class EventLoop;
class Channel : noncopyable{
public:
    typedef std::function<void()> EventCallback;
    typedef std::function<void(Timestamp)> ReadEventCallback;

    Channel(EventLoop* loop,int fd);
    ~Channel();
    void handleEvent(Timestamp receiveTime);
    void setReadCallback(ReadEventCallback cb){
        readCallback_=std::move(cb);
    }
    void setWriteCallback(EventCallback cb){
        writeCallback_=std::move(cb);
    }
    void setErrorCallback(EventCallback cb){
        errorCallback_=std::move(cb);
    }
    void setCloseCallback(const EventCallback& cb)
    { closeCallback_ = cb; }

    int fd() const { return fd_; }
    int events() const { return events_; }
    int set_revents(int revt) { revents_ = revt; }
    bool isNoneEvent() const { return events_ == kNoneEvent; }

    void enableReading() { events_|=kReadEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }
    //for Poller
    int index() { return index_; }
    void set_index(int idx) { index_=idx; }

    EventLoop* ownerLoop() { return loop_; }
private:
    void update();
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_;
    const int fd_;
    int events_;//它关心的IO事件，由用户设置
    int revents_;//目前活动的事件，由Poller设置
    int index_;

    bool eventHandling_;

    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback errorCallback_;
    EventCallback closeCallback_;
};
}//net
}//muduo

#endif