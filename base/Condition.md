### 类 Condition

是muduo库中对系统线程条件变量类函数进行的封装；往往跟mutexlock配合使用，

但也不控制其对象的生存期。

#### 使用规范

线程1 

---

锁住mutex

​	while(条件)

​		wait

解锁mutex

-----

线程2

锁住mutex

​	更改条件

​		signal或broadcast

解锁mutex

----

整个condition类主要为方便用户使用,封装了condition的api，notify() 封装了 pthread_cond_signal

notifyAll() 封装了pthread_cond_broadcast 

wait() 封装了 pthread_cond_wait

waitForSeconds() 封装了 pthread_cond_timewait

#### Condition.h 源码注释

就是简单的封装了api，没有什么可注释的这个文件

```c++
#ifndef MUDUO_BASE_CONDITION_H
#define MUDUO_BASE_CONDITION_H

#include "Mutex.h"

#include <pthread.h>

namespace muduo{

class Condition : noncopyable{
public:
    explicit Condition(MutexLock& mutex):mutex_(mutex){
        pthread_cond_init(&pcond_,NULL);
    }   
    ~Condition(){
        pthread_cond_destroy(&pcond_);
    }
    void wait(){
        MutexLock::UnassignGuard ug(mutex_);
        pthread_cond_wait(&pcond_,mutex_.getPthreadMutex());
    }
    // returns true if time out, false otherwise.
    bool waitForSeconds(double seconds);
    void notify(){
        pthread_cond_signal(&pcond_);
    }
    void notifyAll(){
        pthread_cond_broadcast(&pcond_);
    }
private:
    MutexLock& mutex_;
    pthread_cond_t pcond_;
    
};
}

#endif
```



### 测试程序

这里就简单地构造条件变量。日后看完有关thread的其他部分源码后再进行较为复杂的测试，像CountDownLatch 中用到条件变量来实现倒计时计数器

```c++

#include "Mutex.h"
#include "Condition.h"
#include <sstream>
#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <iostream>
using namespace muduo;

const int N = 10;

int main()
{
    MutexLock mutex;
    Condition pcond(mutex);
    return 0;
}
```


