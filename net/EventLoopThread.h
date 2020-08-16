#ifndef MUDUO_NET_EVENTLOOPTHREAD_H
#define MUDUO_NET_EVENTLOOPTHREAD_H

#include "../base/Condition.h"
#include "../base/Mutex.h"
#include "../base/Thread.h"
#include "../base/noncopyable.h"

namespace muduo
{
namespace net
{
class EventLoop;
class EventLoopThread :noncopyable
{
public:
    EventLoopThread();
    ~EventLoopThread();
    EventLoop* startLoop();//返回EventLoop的指针

private:
    void threadFunc();//thread_的逻辑函数

    EventLoop* loop_;
    bool exiting_;
    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
};

}
}
#endif