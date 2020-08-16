#include "EventLoopThread.h"

#include "EventLoop.h"

#include <boost/bind.hpp>

using namespace muduo;
using namespace muduo::net;
EventLoopThread::EventLoopThread()
  : loop_(NULL),
    exiting_(false),
    thread_(boost::bind(&EventLoopThread::threadFunc, this)),
    mutex_(),
    cond_(mutex_)
{
}
EventLoopThread::~EventLoopThread()
{
  exiting_ = true;
  loop_->quit();
  thread_.join();
}
/*
这个函数是启动线程，并返回线程中EventLoop的指针。函数里使用了条件变量来等待线程的创建。
线程start中执行的逻辑如前所述就是threadFunc
*/
EventLoop* EventLoopThread::startLoop(){
    assert(!thread_.started());
    thread_.start();
    {
        MutexLockGuard lock(mutex_);
        while (loop_ == NULL)
        {
            cond_.wait();
        }
    }
    return loop_;
}
/*
threadFunc中创建一个EventLopp，赋值给&loop,然后唤醒startLoop，然后执行loop函数
*/
void EventLoopThread::threadFunc(){
    EventLoop loop;
    {
        MutexLockGuard lock(mutex_);
        loop_ = &loop;
        cond_.notify();
    }
    loop.loop();
}