CurrentThread主要就是为了获得当前线程的id和获取调用堆栈,Thread封装了创建线程及相关的api

CurrentThread.h中声明的一些函数在Thread.cpp中定义，两者关系紧密，所以可以放在一起看

#### CurrentThread.h 源码注释

```c++
#ifndef MUDUO_BASE_CURRENTTHREAD_H
#define MUDUO_BASE_CURRENTTHREAD_H

#include "Types.h"

namespace  muduo{
//线程局部变量
namespace CurrentThread{
    /*
    extern关键字：用在变量声明中常有这样一个作用，在*.cpp文件中声明一个全局变量,这个全局的变量如果要被引用，
    就放在*.h中并用extern来声明。
    */
    //__thread修饰的变量在每个线程中有独立的实体，各个线程中的变量互不干扰。
    //__thread只能修饰POD类型变量(与C兼容的原始数据)。
    extern __thread int t_cachedTid;//缓存中的线程id
    extern __thread char t_tidString[32];//线程id的字符串形式
    extern __thread int t_tidStringLength;//线程id字符串的长度
    extern __thread const char* t_threadName;//线程名陈
    void cacheTid();//在thread.cpp 中定义
    inline int tid(){//本线程第一次调用的时候才进行系统调用，以后都是直接从thread local缓存的线程id拿到结果
        /*
        这个指令是gcc引入的，作用是允许程序员将最有可能执行的分支告诉编译器。这个指令的写法为：__builtin_expect(EXP, N)。
        意思是：EXP==N的概率很大.
        这是一种性能优化，在这里就表示t_cachedTid为正的概率大，即直接返回t_cachedTid的概率大
        */
        if(__builtin_expect(t_cachedTid==0,0)){
            cacheTid();
        }
        return t_cachedTid;
    }

    inline const char* tidString(){// for logging
        return t_tidString;
    }
    inline int tidStringLength(){// for logging
        return t_tidStringLength;
    }
    inline const char* name(){
        return t_threadName;
    }
    bool isMainThread();//在thread.cpp 中定义
    void sleepUsec(int64_t usec); // for testing
    
    string stackTrace(bool demangle);
}
}

#endif
```



#### CurrentThread.cpp 源码注释

```c++
#include"CurrentThread.h"

#include <cxxabi.h>
#include <execinfo.h>
#include <stdlib.h>
#include <type_traits>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>
namespace  muduo{
//线程局部变量
namespace CurrentThread{
    //__thread修饰的变量在每个线程中有独立的实体，各个线程中的变量互不干扰。
    //__thread只能修饰POD类型变量(与C兼容的原始数据)。
    __thread int t_cachedTid = 0;
    __thread char t_tidString[32];
    __thread int t_tidStringLength = 6;
    __thread const char* t_threadName = "unknown";
    /*
    C++11中引入了static_assert这个关键字，用来做编译期间的断言，因此叫作静态断言
    static_assert(常量表达式，"提示字符串")
    如果第一个参数常量表达式的值为false，会产生一条编译错误。
    错误位置就是该static_assert语句所在行，第二个参数就是错误提示字符串。
    性能方面：由于static_assert是编译期间断言，不生成目标代码，因此static_assert不会造成任何运行期性能损失
    */
    /*
    template< class T, class U >
    struct is_same;
    如果T与U具有同一const-volatile限定的相同类型，则is_same<T,U>::value为true，否则为false。
    要求pid_t必须是int类型
    */
    static_assert(std::is_same<int, pid_t>::value, "pid_t should be int");
    /*void cacheTid(){
        if (t_cachedTid == 0)
        {
            t_cachedTid=static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }*/
    /*
    该函数是用来获取调用堆栈，debug用
    首先，关于调用堆栈:假设我们有几个函数，fun1，fun2，fun3，fun4，且fun1调用fun2，fun2调用fun3，fun3调用fun4.
    在fun4运行过程中，我能可以从线程当前堆栈中了解到调用它的几个函数分别是谁。
    从函数的顺序来看，fun4,fun3,fun2,fun1呈现出一种“堆栈”的特征，最后被调用的函数出现在最上方，
    因此称这种关系为调用堆栈（call stack）

    使用场景：
    当故障发生时，如果程序被中断，我们基本只可以看到最后出错的函数。
    利用call stack，我们可以知道当出错函数被谁调用的时候出错。这样一层层看上去，可以猜测出错误的原因
    */
    string stackTrace(bool demangle){
        string stack;
        const int max_frames = 200;
        void* frame[max_frames];
        /*
        int backtrace (void **buffer, int size);
        该函数用来获取当前线程的调用堆栈，获取的信息将会被存放在buffer中，它是一个指针数组。
        参数size用来指定buffer中可以保存多少个void* 元素。
        函数返回值是实际获取的指针个数，最大不超过size大小在buffer中的指针实际是从堆栈中获取的返回地址,
        每一个堆栈框架有一个返回地址。
        注意某些编译器的优化选项对获取正确的调用堆栈有干扰，另外内联函数没有堆栈框架；删除框架指针也会使无法正确解析堆栈内容。
        */
        int nptrs = ::backtrace(frame, max_frames);
        /*
        char **backtrace_symbols (void *const *buffer, int size);

        该函数将从backtrace函数获取的信息转化为一个字符串数组。
        参数buffer是从backtrace函数获取的数组指针，size是该数组中的元素个数(backtrace的返回值)，
        函数返回值是一个指向字符串数组的指针,它的大小同buffer相同。
        每个字符串包含了一个相对于buffer中对应元素的可打印信息。
        它包括函数名，函数的偏移地址和实际的返回地址。
        backtrace_symbols生成的字符串都是malloc出来的，但是不要最后一个一个的free，
        因为backtrace_symbols会根据backtrace给出的callstack层数，一次性的将malloc出来一块内存释放，
        所以，只需要在最后free返回指针就OK了。
        */
        char** strings = ::backtrace_symbols(frame, nptrs);
        if (strings)
        {
            size_t len = 256;
            char* demangled = demangle ? static_cast<char*>(::malloc(len)) : nullptr;
            for (int i = 1; i < nptrs; ++i)  // skipping the 0-th, which is this function
            {
                if (demangle)
                {
                    // https://panthema.net/2008/0901-stacktrace-demangled/
                    // bin/exception_test(_ZN3Bar4testEv+0x79) [0x401909]
                    char* left_par = nullptr;
                    char* plus = nullptr;
                    for (char* p = strings[i]; *p; ++p)
                    {
                        if (*p == '(')
                            left_par = p;
                        else if (*p == '+')
                        plus = p;
                    }

                    if (left_par && plus)
                    {
                        *plus = '\0';
                        int status = 0;
                        char* ret = abi::__cxa_demangle(left_par+1, demangled, &len, &status);
                        *plus = '+';
                        if (status == 0)
                        {
                            demangled = ret;  // ret could be realloc()
                            stack.append(strings[i], left_par+1);
                            stack.append(demangled);
                            stack.append(plus);
                            stack.push_back('\n');
                            continue;
                        }
                    }
                }
                // Fallback to mangled names
                stack.append(strings[i]);
                stack.push_back('\n');
            }
            free(demangled);
            free(strings);
        }
        return stack;
    }
}//CurrentThread
}//muduo
```



#### Thread.h 源码注释

```c++
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
```

#### Thread.cpp 源码注释

```c++
#include "Thread.h"
#include "CurrentThread.h"
#include "Exception.h"
#include "Logging.h"

#include <type_traits>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

namespace muduo
{
namespace detail
{

pid_t gettid(){
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

void afterFork()
{
    muduo::CurrentThread::t_cachedTid = 0;
    muduo::CurrentThread::t_threadName = "main";
    CurrentThread::tid();
    // no need to call pthread_atfork(NULL, NULL, &afterFork);
}
class ThreadNameInitializer
{
public:
    ThreadNameInitializer()
    {
        muduo::CurrentThread::t_threadName = "main";
        CurrentThread::tid();
        /*
        int pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void));

        pthread_atfork()在fork()之前调用，当调用fork时，内部创建子进程前在父进程中会调用prepare，
        内部创建子进程成功后，父进程会调用parent ，子进程会调用child。
        */
        pthread_atfork(NULL, NULL, &afterFork);
    }
};

ThreadNameInitializer init;
//线程创建的函数的参数类型就是这个结构体，另外，这个结构体中的runInThread()是真正的线程函数，被线程函数startThread() 调用
struct ThreadData
{
    typedef muduo::Thread::ThreadFunc ThreadFunc;
    ThreadFunc func_;
    string name_;
    pid_t* tid_;
    CountDownLatch* latch_;

    ThreadData(ThreadFunc func,
             const string& name,
             pid_t* tid,
             CountDownLatch* latch)
    : func_(std::move(func)),
      name_(name),
      tid_(tid),
      latch_(latch)
    { }

    void runInThread()
    {
        *tid_ = muduo::CurrentThread::tid();
        tid_ = NULL;
        latch_->countDown();
        latch_ = NULL;

        muduo::CurrentThread::t_threadName = name_.empty() ? "muduoThread" : name_.c_str();
        ::prctl(PR_SET_NAME, muduo::CurrentThread::t_threadName);
        try
        {
            func_();
            muduo::CurrentThread::t_threadName = "finished";
        }
        catch (const Exception& ex)
        {
            muduo::CurrentThread::t_threadName = "crashed";
            fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
            fprintf(stderr, "reason: %s\n", ex.what());
            fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
            abort();
        }
        catch (const std::exception& ex)
        {
            muduo::CurrentThread::t_threadName = "crashed";
            fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
            fprintf(stderr, "reason: %s\n", ex.what());
            abort();
        }
        catch (...)
        {
            muduo::CurrentThread::t_threadName = "crashed";
            fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
            throw; // rethrow
        }
    }
};

void* startThread(void* obj)
{
    ThreadData* data = static_cast<ThreadData*>(obj);
    data->runInThread();
    delete data;
    return NULL;
}

}//detail
void CurrentThread::cacheTid()
{
    if (t_cachedTid == 0)
    {
        t_cachedTid = detail::gettid();
        t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
    }
}

bool CurrentThread::isMainThread()
{
    return tid() == ::getpid();
}

void CurrentThread::sleepUsec(int64_t usec)
{
    struct timespec ts = { 0, 0 };
    ts.tv_sec = static_cast<time_t>(usec / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(usec % Timestamp::kMicroSecondsPerSecond * 1000);
    ::nanosleep(&ts, NULL);
}

AtomicInt32 Thread::numCreated_;
/*
线程池中调用Thread构造函数如下：

//将this分配给runInThread,相当于构造Thread(this.runInThread,name+id)并加入线程数组。线程函数是runInThread 
threads_.push_back(new muduo::Thread(
          boost::bind(&ThreadPool::runInThread, this), name_+id));	
 
    threads_[i].start();	//启动每个线程，但是由于线程运行的函数是runInThread，所以会阻塞

线程函数即为：this.runInThread,线程名为name+id。  (注意此处的runInThread不是Thread类中的,而是ThreadPool中的)
*/
Thread::Thread(ThreadFunc func, const string& n)
  : started_(false),
    joined_(false),
    pthreadId_(0),
    tid_(0),
    func_(std::move(func)),
    name_(n),
    latch_(1)
{
    setDefaultName();
}

Thread::~Thread()
{
  if (started_ && !joined_)
  {
    pthread_detach(pthreadId_);
  }
}

void Thread::setDefaultName()
{
    int num = numCreated_.incrementAndGet();
    //如果线程创建参数中没有线程名，即name_为空，则将name_赋值为 "Thread+id" id就是创建的第几个线程
    if (name_.empty())
    {
        char buf[32];
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}

/*
线程启动函数，调用pthread_create创建线程，线程函数为detail::startThread，
传递给线程函数的参数data是在heap上分配的，data存放了线程真正要执行的函数记为func、线程id、线程name等信息。
detail::startThread会调用func启动线程，所以detail::startThread可以看成是一个跳板或中介。
*/
void Thread::start()//线程启动函数,调用pthread_create创建线程
{
    assert(!started_);//确保线程没有启动
    started_ = true;//设置标记，线程已经启动
    // FIXME: move(func_)
    detail::ThreadData* data = new detail::ThreadData(func_, name_, &tid_, &latch_);
    if (pthread_create(&pthreadId_, NULL, &detail::startThread, data))
    {
        started_ = false;//创建线程失败,设置标记线程未启动
        delete data; // or no delete?
        LOG_SYSFATAL << "Failed in pthread_create";
    }
    else
    {
        latch_.wait();
        assert(tid_ > 0);
    }
}

int Thread::join()
{
    assert(started_);
    assert(!joined_);
    joined_ = true;
    return pthread_join(pthreadId_, NULL);
}

}//muduo
```



### 测试程序

关于测试程序中的boost::bind()：

可以支持函数对象、函数、函数指针、成员函数指针，并且绑定任意参数到某个指定值上或者将输入参数传入任意位置。bind(f, 1, 2)等价于f(1, 2); bind(g, 1, 2, 3)等价于g(1, 2, 3);

一般情况下，bind 与 function 配合使用。

```c++
#include "Thread.h"
#include "CurrentThread.h"
 
#include <string>
#include <boost/bind.hpp>
#include <stdio.h>
#include <unistd.h>
 
void mysleep(int seconds)
{
    timespec t = { seconds, 0 };
    nanosleep(&t, NULL);
}
 
void threadFunc()
{
    printf("tid=%d\n", muduo::CurrentThread::tid());
}
 
void threadFunc2(int x)
{
    printf("tid=%d, x=%d\n", muduo::CurrentThread::tid(), x);
}
 
void threadFunc3()
{
    printf("tid=%d\n", muduo::CurrentThread::tid());
    mysleep(1);
}
 
class Foo
{
public:
    explicit Foo(double x)
    : x_(x)
    {
    }
 
    void memberFunc()
    {
        printf("tid=%d, Foo::x_=%f\n", muduo::CurrentThread::tid(), x_);
    }
 
    void memberFunc2(const std::string& text)
    {
        printf("tid=%d, Foo::x_=%f, text=%s\n", muduo::CurrentThread::tid(), x_, text.c_str());
    }
 
private:
    double x_;
};
 
int main()
{
    printf("pid=%d, tid=%d\n", ::getpid(), muduo::CurrentThread::tid());
 
    muduo::Thread t1(threadFunc);
    t1.start();
    printf("t1.tid=%d\n", t1.tid());
    t1.join();
//bind绑定普通函数
    muduo::Thread t2(boost::bind(threadFunc2, 42),
                   "thread for free function with argument");
    t2.start();
    printf("t2.tid=%d\n", t2.tid());
    t2.join();
//bind绑定成员函数
    Foo foo(87.53);
    muduo::Thread t3(boost::bind(&Foo::memberFunc, &foo),
                   "thread for member function without argument");
    t3.start();
    t3.join();
 
    muduo::Thread t4(boost::bind(&Foo::memberFunc2, boost::ref(foo), std::string("Shuo Chen")));
    t4.start();
    t4.join();
 
    printf("number of created threads %d\n", muduo::Thread::numCreated());
}
```

测试结果

```c++
knopfler@DESKTOP-3UDOCBE:~/muduo/base$ make
g++ -o test test.cpp Mutex.h Condition.h Condition.cpp CurrentThread.h CurrentThread.cpp CountDownLatch.h CountDownLatch.cpp Atomic.h Thread.h Thread.cpp Timestamp.h Timestamp.cpp Logging.h Logging.cpp TimeZone.h TimeZone.cpp LogStream.h LogStream.cpp Date.h Date.cpp -lpthread
knopfler@DESKTOP-3UDOCBE:~/muduo/base$ ./test
pid=7171, tid=7171
t1.tid=7172
tid=7172
tid=7173, x=42
t2.tid=7173
tid=7174, Foo::x_=87.530000
tid=7175, Foo::x_=87.530000, text=Shuo Chen
number of created threads 4
```

