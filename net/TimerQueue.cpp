#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include "../net/TimerQueue.h"

#include "../base/Logging.h"
#include "EventLoop.h"
#include "Timer.h"
#include "TimerId.h"

#include <sys/timerfd.h>
#include <unistd.h>
#include <boost/bind.hpp>
namespace muduo
{
namespace net
{
namespace detail
{
int createTimerfd(){
    /*
    timerfd是Linux为用户程序提供的一个定时器接口。这个接口基于文件描述符，
    通过文件描述符的可读事件进行超时通知，所以能够被用于select/poll的应用场景。
    TFD_NONBLOCK 表示非阻塞的文件描述符
    TFD_CLOEXEC 表示fork或者exec后自动关闭，不会继承父进程的打开状态
    CLOCK_MONOTONIC 是单调时间，即从某个时间点开始到现在过去的时间，用户不能修改这个时间
    */
    // 创建一个 时间 fd
    int timerfd=::timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK|TFD_CLOEXEC);
    if(timerfd<0){
        LOG_SYSFATAL << "Failed in timerfd_create";
    }
    return timerfd;
}
//获得定时器到时时间与现在时间的差值
struct timespec howMuchTimeFromNow(Timestamp when){
    int64_t microseconds = when.microSecondsSinceEpoch()
                         - Timestamp::now().microSecondsSinceEpoch();
    if (microseconds < 100)
    {
        microseconds = 100;
    }
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    return ts;
}
void readTimerfd(int timerfd, Timestamp now){
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
    if (n != sizeof howmany)
    {
        LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
    }
}
void resetTimerfd(int timerfd,Timestamp expiration){
    /*
    struct itimerspec 
    {
        struct timespec it_interval;   //Timer interval(timer循环时间间隔) 
        struct timespec it_value;      //Initial expiration(timer初次到期时间间隔) 
    };
    struct timespec 
    {
        time_t tv_sec;        // Seconds 
        long   tv_nsec;        // Nanoseconds(纳秒:十亿分之一秒) 
    };
    */
    struct itimerspec newValue;
    struct itimerspec oldValue;
    bzero(&newValue,sizeof(newValue));
    bzero(&oldValue,sizeof(oldValue));
    newValue.it_value=howMuchTimeFromNow(expiration);
    /*
    int timerfd_settime(int fd, int flags, const struct itimerspec *new_value, struct itimerspec *old_value);

    timerfd_settime()此函数用于设置新的超时时间，并开始计时,能够启动和停止定时器;
    fd: 参数fd是timerfd_create函数返回的文件句柄
    flags：参数flags为1代表设置的是绝对时间（TFD_TIMER_ABSTIME 表示绝对定时器）；为0代表相对时间。
    new_value: 参数new_value指定定时器的超时时间以及超时间隔时间
    old_value: 如果old_value不为NULL, old_vlaue返回之前定时器设置的超时时间，具体参考timerfd_gettime()函数

    ** it_interval不为0则表示是周期性定时器。
       it_value和it_interval都为0表示停止定时器

    */
    int ret=::timerfd_settime(timerfd,0,&newValue,&oldValue);
    if(ret){
        LOG_SYSERR << "timerfd_settime()";
    }
}

}//detail
}//net
}//muduo
using namespace muduo;
using namespace muduo::net;
using namespace muduo::net::detail;

TimerQueue::TimerQueue(EventLoop* loop)
    :loop_(loop),
    timerfd_(createTimerfd()),
    timerfdChannel_(loop,timerfd_),
    timers_()
{
    timerfdChannel_.setReadCallback(boost::bind(&TimerQueue::handleRead,this));
    timerfdChannel_.enableReading();
}
TimerQueue::~TimerQueue(){
    ::close(timerfd_);
    for(TimerList::iterator it=timers_.begin();it!=timers_.end();++it){
        delete it->second;
    }
}
/*
 * 用户调用runAt/runAfter/runEveny后由EventLoop调用的函数
 * 向时间set中添加时间
 * 
 * @param cb，用户提供的回调函数，当时间到了会执行
 * @param when，超时时间，绝对时间
 * @param interval，是否调用runEveny，即是否是永久的，激活一次后是否继续等待
 * 
 * std::move,避免拷贝，移动语义
 * std::bind,绑定函数和对象，生成函数指针
 */
TimerId TimerQueue::addTimer(const TimerCallback& cb,Timestamp when,double interval){
    Timer* timer=new Timer(std::move(cb),when,interval);
    loop_->runInLoop(boost::bind(&TimerQueue::addTimerInLoop,this,timer));
    return TimerId(timer);
}
//完成修改定时器列表的工作
void TimerQueue::addTimerInLoop(Timer* timer){
    loop_->assertInLoopThread();
    bool earliestChanged=insert(timer);
    if(earliestChanged){
        resetTimerfd(timerfd_,timer->expiration());
    }
}
/*
当定时器超时，保存timerfd的Channel激活，调用回调函数
*/
void TimerQueue::handleRead(){
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_,now);
    std::vector<Entry>expired=getExpired(now) ;
    for(std::vector<Entry>::iterator it=expired.begin();it!=expired.end();++it){
        it->second->run();
    }
    reset(expired,now);
}
/*
从timers_中移除已到期的Timer，并通过vector返回他们
*/
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now){
    std::vector<Entry>expired;
    //哨兵值(sentry)让lower_bound() 返回的是第一个未到期的Timer的迭代器
    Entry sentry=std::make_pair(now,reinterpret_cast<Timer*>(UINTPTR_MAX));
    TimerList::iterator it=timers_.lower_bound(sentry);
    assert(it==timers_.end()||now<it->first);
    /*
    copy函数的函数原型: 
    //fist [IN]: 要拷贝元素的首地址
    //last [IN]:要拷贝元素的最后一个元素的下一个地址
    //x [OUT] : 拷贝的目的地的首地址
    template<class InIt, class OutIt>
        OutIt copy(InIt first, InIt last, OutIt x);
    
    如果要把一个序列（sequence）拷贝到一个容器（container）中去，通常用std::copy算法，代码如下：
    std::copy(start, end, std::back_inserter(container));
    */
    std::copy(timers_.begin(),it,back_inserter(expired));
    timers_.erase(timers_.begin(),it);
    return expired;
}
//调用完回调函数之后需要将周期性任务重新添加到set中，要重新计算超时时间
void TimerQueue::reset(const std::vector<Entry>& expired,Timestamp now){
    Timestamp nextExpire;
    for(std::vector<Entry>::const_iterator it=expired.begin();it!=expired.end();++it){
        //是否为周期性任务
        if(it->second->repeat()){

            it->second->restart(now);
            insert(it->second);
        }
        else{
            delete it->second;
        }
    }
    /* 计算下次timerfd被激活的时间 */
    if(!timers_.empty()){
        nextExpire = timers_.begin()->second->expiration();
    }
    if(nextExpire.valid()){
        resetTimerfd(timerfd_, nextExpire);
    }

}

bool TimerQueue::insert(Timer* timer){
    //最新的超时时间
    bool earliestChanged =false;
    /* 获取timer的UTC时间戳，和timer组成std::pair<Timestamp, Timer*> */
    Timestamp when=timer->expiration();
    //取得超时时间最近的Timer
    TimerList::iterator it=timers_.begin();
    /* 如果要添加的timer的超时时间比timers_中的超时时间近，更改新的超时时间 */
    if(it==timers_.end()||when<it->first){
        earliestChanged=true;
    }
    /* 
    添加到定时任务的set中 
    set的单元素版返回一个二元组（Pair）。成员 pair::first 被设置为指向新插入元素的迭代器或指向等值的已经存在的元素的迭代器。
    成员 pair::second 是一个 bool 值，如果新的元素被插入，返回 true，
    如果等值元素已经存在（即无新元素插入），则返回 false。　
    */
    std::pair<TimerList::iterator,bool>result=timers_.insert(std::make_pair(when,timer));
    return earliestChanged;
}