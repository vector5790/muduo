#ifndef MUDUO_BASE_THREAD_H
#define MUDUO_BASE_THREAD_H

#include "Atomic.h"
#include "CountDownLatch.h"
#include "Types.h"

#include <functional>
#include <memory>
#include <pthread.h>

namespace muduo
{
class Thread: noncopyable{
public:
    //定义回调函数
    typedef std::function<void()> ThreadFunc;
    explicit Thread(ThreadFunc,const string& name=string());
    ~Thread();
    //开启线程的接口
    void start();
    //等待线程结束
    int join();
    bool started() const {return started_;}
    //返回线程id
    pid_t tid() const { return tid_; }
    //返回线程名
    const string& name() const { return name_; }
    //已经启动的线程个数
    static int numCreated() { return numCreated_.get(); }
private:
    void setDefaultName();
    bool started_;//启动标识，表示线程是否启动
    bool joined_;
    /*
    pthreadId_是线程ID，但是这个值不是唯一的，在不同进程下的两个线程可能会有同一个线程ID，
    当出现进程p1中的线程t1要与进程p2中的线程t2通信的情况时，需要一个真实的线程id唯一标识，即tid。
    glibc没有实现gettid的函数，可以通过linux下的系统调用syscall(SYS_gettid)来获得
    */
    pthread_t pthreadId_;
    //该线程的真实id，可以唯一标识一个线程
    pid_t tid_;
    //真正调用的回调函数
    ThreadFunc func_;
    //线程名称
    string name_;
    CountDownLatch latch_;
    //统计当前线程数
    static AtomicInt32 numCreated_;
};

}//muduo

#endif