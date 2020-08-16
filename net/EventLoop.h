/*
事件分发器
*/
#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include <atomic>
#include <functional>
#include <vector>

#include <boost/any.hpp>

#include "../base/Mutex.h"
#include "../base/CurrentThread.h"
#include "../base/Timestamp.h"
#include "../base/Thread.h"
#include "Callbacks.h"
#include "TimerId.h"
#include <boost/scoped_ptr.hpp>
namespace muduo{

namespace net{

class Channel;
class Poller;
class TimerQueue;

class EventLoop : noncopyable{
public:
    typedef boost::function<void()> Functor;
    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    void runInLoop(const Functor& cb);
    void queueInLoop(const Functor& cb);

     ///
    /// Time when poll returns, usually means data arrivial.
    ///
    Timestamp pollReturnTime() const { return pollReturnTime_; }
    
    // timers
    ///
    /// Runs callback at 'time'.
    ///
    TimerId runAt(const Timestamp& time, const TimerCallback& cb);
    ///
    /// Runs callback after @c delay seconds.
    ///
    TimerId runAfter(double delay, const TimerCallback& cb);
    ///
    /// Runs callback every @c interval seconds.
    ///
    TimerId runEvery(double interval, const TimerCallback& cb);
    
    void wakeup();
    void updateChannel(Channel* channel);

    void assertInLoopThread(){
        if(!isInLoopThread()){
            abortNotInLoopThread();
        }
    }

    bool isInLoopThread() const { return threadId_==CurrentThread::tid(); }
    /*
    既然每个线程至多有一个EventLoop对象，那么我们让EventLoop的static成员函数getEventLoopOfCurrentThread()
    返回这个对象。返回值可能为NULL，如果当前线程不是IO线程的话。
    */
    static EventLoop* getEventLoopOfCurrentThread();
private:
    void abortNotInLoopThread();
    void handleRead();//waked up
    void doPendingFunctors();

    typedef std::vector<Channel*> ChannelList;

    bool looping_;
    bool quit_;
    bool callingPendingFunctors_;
    const pid_t threadId_;
    Timestamp pollReturnTime_;
    boost::scoped_ptr<Poller>poller_;
    boost::scoped_ptr<TimerQueue> timerQueue_;
    int wakeupFd_;
    /*
    用于处理wakeupFd_上的readable事件，将事件分发至handleRead()函数。其中只有pendingFunctors_暴露给其他线程，
    因此用mutex保护
    */
    boost::scoped_ptr<Channel> wakeupChannel_;
    ChannelList activeChannels_;
    MutexLock mutex_;
    std::vector<Functor> pendingFunctors_; // @GuardedBy mutex_
};

}//net
}//muduo

#endif