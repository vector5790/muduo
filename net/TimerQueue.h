#ifndef MUDUO_NET_TIMERQUEUE_H
#define MUDUO_NET_TIMERQUEUE_H

#include <set>
#include <vector>

#include <boost/noncopyable.hpp>

#include "../base/Timestamp.h"
#include "../base/Mutex.h"
#include "Callbacks.h"
#include "Channel.h"

namespace muduo
{
namespace net{
class EventLoop;
class Timer;
class TimerId;

class TimerQueue : noncopyable{
public:
    TimerQueue(EventLoop* loop);
    ~TimerQueue();
    /* 
   * 用于注册定时任务
   * @param cb, 超时调用的回调函数
   * @param when，超时时间(绝对时间)
   * @interval，是否是周期性超时任务
   */
    TimerId addTimer(const TimerCallback& cb,Timestamp when,double interval);
private:
    typedef std::pair<Timestamp, Timer*> Entry;
    typedef std::set<Entry> TimerList;

    void handleRead();
    std::vector<Entry>getExpired(Timestamp now);
     /* 将超时任务中周期性的任务重新添加到timers_中 */
    void reset(const std::vector<Entry>& expired,Timestamp now);
    /* 插入到timers_中 */
    bool insert(Timer* timer);
    /* 所属的事件驱动循环 */
    EventLoop* loop_;
    /* 由timerfd_create创建的文件描述符 */
    const int timerfd_;
    /* 用于监听timerfd的Channel */
    Channel timerfdChannel_;
    /* 保存所有的定时任务 */
    TimerList timers_;

};

}//net
}//muduo

#endif