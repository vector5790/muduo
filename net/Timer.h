#ifndef MUDUO_NET_TIMER_H
#define MUDUO_NET_TIMER_H

#include "../base/Atomic.h"
#include "../base/Timestamp.h"
#include "Callbacks.h"

namespace muduo
{
namespace net
{

///
/// Internal class for timer event.
///
class Timer : noncopyable{
public:
    Timer(const TimerCallback& cb,Timestamp when,double interval)
    :callback_(cb),
    expiration_(when),
    interval_(interval),
    repeat_(interval>0.0)
    { 
    }
    void run() const {
        callback_();
    }
    Timestamp expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }
    /* 重新计算超时时间 */
    void restart(Timestamp now);
    
private:
    const TimerCallback callback_;
    Timestamp expiration_;
    const double interval_;
    const bool repeat_;
};
}//net
}//muduo
#endif