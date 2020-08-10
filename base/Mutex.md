### 互斥器

Mutex.h中只用到了CurrentThread中的获取当前线程id的方法,就下面一句

`holder_ = CurrentThread::tid();`

其实就是调用了系统调用syscall来获取线程id。

```c++
static_cast<pid_t>(::syscall(SYS_gettid))
```

所以可以先不用看CurrentThread的源代码，只要看当前Mutex.h的源代码即可



Mutex的使用方法可以看注释里，或者测试程序

#### Mutex.h 源码注释

```c++
#ifndef MUDUO_BASE_MUTEX_H
#define MUDUO_BASE_MUTEX_H

#include "CurrentThread.h"
#include "noncopyable.h"
#include <assert.h>
#include <pthread.h>

namespace muduo{
class MutexLock : noncopyable{
public:
    MutexLock():holder_(0){
        //MEMCHECK是多retval的检测，相当于assert，下同
        pthread_mutex_init(&mutex_,NULL);
    }
    ~MutexLock(){
        assert(holder_ == 0);//只有在没有被其它线程持有的情况下才可以析构
        pthread_mutex_destroy(&mutex_);
    }
    // must be called when locked, i.e. for assertion
    bool isLockedByThisThread() const{//是否被本线程上锁
        return holder_ == CurrentThread::tid();
    }

    void assertLocked() const {
        assert(isLockedByThisThread());
    }
    void lock(){ //仅供MutexLockGuard 调用,严禁用户代码调用
        pthread_mutex_lock(&mutex_);
        assignHolder();//赋值，赋上tid
    }
    void unlock(){ //仅供MutexLockGuard 调用,严禁用户代码调用
        unassignHolder();//首先要清零
        pthread_mutex_unlock(&mutex_);
    }
    //仅供 Condition 调用,严禁用户代码调用
    pthread_mutex_t* getPthreadMutex(){ /* non-const */
        return &mutex_;
    }
private:
    void unassignHolder(){
        holder_ = 0;
    }
    void assignHolder(){
        holder_ = CurrentThread::tid();
    }

    friend class Condition;
    // 类中类，守护未被分配； 创建 UnassignGuard 传入 MutexLock 构造后，将 holder清除
    class UnassignGuard : noncopyable{ //取消赋值
    public:
        explicit UnassignGuard(MutexLock& owner):owner_(owner){
            owner_.unassignHolder();
        }
        ~UnassignGuard(){
            owner_.assignHolder();
        }
    private:
        MutexLock& owner_;
    };
    //互斥锁
    pthread_mutex_t mutex_;
    //进程id
    pid_t holder_;
};
//我们用这个类，就是在利用C++的RAII机制，让锁在作用域内全自动化
class MutexLockGuard : noncopyable{
public:
    /*
    C++中的explicit关键字只能用于修饰只有一个参数的类构造函数, 它的作用是表明该构造函数是显示的, 而非隐式的, 
    跟它相对应的另一个关键字是implicit, 意思是隐藏的,类构造函数默认情况下即声明为implicit(隐式).
    explicit关键字的作用就是防止类构造函数的隐式自动转换.
    eg. https://blog.csdn.net/guoyunfei123/article/details/89003369
    */
    explicit MutexLockGuard(MutexLock& mutex):mutex_(mutex){
        mutex_.lock();
    }
    ~MutexLockGuard(){
        mutex_.unlock();
    }
private:
    MutexLock& mutex_;//使用引用不会导致MutexLock对象的销毁
};
}
/*
该宏作用是防止程序里出现如下错误
void doit(){
    MutexLockGuard(mutex);//遗漏变量名，产生一个临时对象又马上销毁了《
    //正确写法是 MutexLockGuard lock(mutex);

}
*/
#define MutexLockGuard(x) static_assert(false,"missing mutex guard var game");
#endif
```



### 测试程序

使用锁实现两个线程交叉打印0-10

```c++
#include "Mutex.h"

#include <sstream>
#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <iostream>
using namespace muduo;

const int N = 10;
MutexLock mutex;
int x=0;
void* fun1(void* arg){   

    while(x<=N){
        if(x%2==1){
          MutexLockGuard lock(mutex);
          std::cout<<"1: "<<x<<std::endl;
          x++;
        }
    }
} 
void* fun2(void* arg){   
    while(x<=N){
        if(x%2==0){
          MutexLockGuard lock(mutex);
          std::cout<<"2: "<<x<<std::endl;
          x++;
        }
    }
} 
int main()
{
    
    pthread_t pid[2];
    pthread_create(&pid[0],NULL,fun1,NULL);
    pthread_create(&pid[1],NULL,fun2,NULL);
    for(int i=0;i<2;i++){
      pthread_join(pid[0],NULL);
    }
    return 0;
}
```

测试结果

```c++
knopfler@DESKTOP-3UDOCBE:~/muduo/base$ make
g++ -o test test.cpp Mutex.h CurrentThread.h CurrentThread.cpp -lpthread
knopfler@DESKTOP-3UDOCBE:~/muduo/base$ ./test
2: 0
1: 1
2: 2
1: 3
2: 4
1: 5
2: 6
1: 7
2: 8
1: 9
2: 10
```

