#include "EventLoop.h"
#include "Channel.h"
#include "Poller.h"
#include "TimerQueue.h"
#include "../base/Logging.h"
#include "../base/Mutex.h"
#include <boost/bind.hpp>
#include <algorithm>

#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <poll.h>

using namespace muduo;
using namespace muduo::net;

__thread EventLoop* t_loopInThisThread=0;
const int kPollTimeMs = 10000;
/*
eventfd是Linux 2.6提供的一种系统调用，它可以用来实现事件通知。eventfd包含一个由内核维护的64位无符号整型计数器，
创建eventfd时会返回一个文件描述符，进程可以通过对这个文件描述符进行read/write来读取/改变计数器的值，
从而实现进程间通信
*/
static int createEventfd(){
    /*
    eventfd()函数原型
    int eventfd(unsignedint initval,int flags);
    initval：创建eventfd时它所对应的64位计数器的初始值；
    flags：eventfd文件描述符的标志，可由三种选项组成：EFD_CLOEXEC、EFD_NONBLOCK和EFD_SEMAPHORE。
    EFD_CLOEXEC表示返回的eventfd文件描述符在fork后exec其他程序时会自动关闭这个文件描述符；
    EFD_NONBLOCK设置返回的eventfd非阻塞；
    EFD_SEMAPHORE表示将eventfd作为一个信号量来使用。
    */
    int evtfd=::eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd<0){
        LOG_SYSERR << "Failed in eventfd";
        abort();
    }
    return evtfd;
}
EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
  return t_loopInThisThread;
}

EventLoop::EventLoop()
    : looping_(false),
    quit_(false),
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),
    poller_(new Poller(this)),
    timerQueue_(new TimerQueue(this)),
    wakeupFd_(createEventfd()),
    wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_TRACE<<"EventLoop created" <<this<<"in thread"<<threadId_;
    if(t_loopInThisThread){
        LOG_FATAL<<"Another EventLoop "<<t_loopInThisThread<<"exists in this thread "<<threadId_;
    }
    else{
        t_loopInThisThread=this;
    }
    wakeupChannel_->setReadCallback(
      boost::bind(&EventLoop::handleRead, this));
    // we are always reading the wakeupfd
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop(){
    assert(!looping_);
    ::close(wakeupFd_);
    t_loopInThisThread=NULL;   
}

void EventLoop::loop(){
    assert(!looping_);
    assertInLoopThread();
    looping_ =true;
    quit_=false;

    while(!quit_){
        activeChannels_.clear();
        poller_->poll(kPollTimeMs,&activeChannels_);
        for(ChannelList::iterator it=activeChannels_.begin();it!=activeChannels_.end();++it){
            (*it)->handleEvent();
        }
        doPendingFunctors();
    }

    LOG_TRACE<<"EventLoop "<<this<<"stop looping";
    looping_=false;
}

void EventLoop::quit()
{
  quit_ = true;
  if(!isInLoopThread()){
      wakeup();
  }
  
}
/*
如果用户在当前IO线程调用这个函数，回调会同步进行；
如果用户在其他线程调用runInLoop()，cb会被加入队列，IO线程会被唤醒来调用这个Functor
*/
void EventLoop::runInLoop(const Functor& cb)
{
    if (isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(cb);
    }
}
void EventLoop::queueInLoop(const Functor& cb){
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.push_back(cb);
    }
    /*
    根据条件判定是否需要唤醒IO线程
    （1）在非IO线程中执行了queueInLoop，因为IO线程有可能正在阻塞在poll中。
    （2）正在调用容器中的cb,这时我们唤醒的目的在于为poll中的文件描述符写入事件
    */
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}
TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb){
    return timerQueue_->addTimer(cb, time, 0.0);
}
TimerId EventLoop::runAfter(double delay, const TimerCallback& cb){
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, cb);
}
TimerId EventLoop::runEvery(double interval, const TimerCallback& cb){
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(cb, time, interval);
}
void EventLoop::updateChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  poller_->updateChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
    LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << threadId_
            << ", current thread id = " <<  CurrentThread::tid();
}
/*
wakeup的实现是使用了eventFd，在EventLoop初始化时将它加入poller，
如果需要唤醒，只需对eventFd写入信息使poller监听到即可
*/
void EventLoop::wakeup()
{
	uint64_t one = 1;
	//ssize_t n = sockets::write(wakeupFd_,&one,sizeof one);
	ssize_t n = ::write(wakeupFd_,&one,sizeof one);
	if(n != sizeof one)
	{
		LOG_ERROR << "EventLoop::wakeup() writes " << n << "bytes instead of 8";
	}
}
void EventLoop::handleRead(){
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}
/*
不是简单地在临界区依次调用Functor，而是把回调列表swap()到局部变量functors中，
这样一方面减小了临界区的长度(意味着不会阻塞其他线程调用queueInLoop()),
另一方面也避免了死锁(因为Functor可能在调用queueInLoop())
*/
void EventLoop::doPendingFunctors(){
    std::vector<Functor>functors;
    callingPendingFunctors_=true;
    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (size_t i = 0; i < functors.size(); ++i)
    {
        functors[i]();
    }
    callingPendingFunctors_ = false;
}