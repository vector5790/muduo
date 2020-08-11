### 类 CountDownLatch

倒计时计数器



#### CountDownLatch.h 源码注释

```c++
/*
倒计时计数器

1. 首先，创建一个计数器，设置 count 的初始值，程序中的数值设置为3；

2. 在执行了线程 D 之后，调用 countDownLatch.wait()方法，将会进入阻塞状态，直到 countDownLatch 的 count 参数值为 0；

3. 在其他线程里，调用 countDownLatch.countDown() 方法，调用该方法会将计数值 减 1；

4. 当其他线程中的 countDown() 方法把计数值变成 0 时，等待线程里的 countDownLatch.wait() 立即退出，执行下面的代码。
*/
#ifndef MUDUO_BASE_COUNTDOWNLATCH_H
#define MUDUO_BASE_COUNTDOWNLATCH_H

#include "Condition.h"
#include "Mutex.h"

namespace muduo
{
class CountDownLatch  : noncopyable{
public:
    explicit CountDownLatch(int count);
    //等待计数为0
    void wait();
    //计数减1
    void CountDown();
    //获得计数
    int getCount() const;
private:
    mutable MutexLock mutex_;
    /*
    GUARDED_BY ----> THREAD_ANNOTATION_ATTRIBUTE__ ----> __attribute__
    guarded_by属性是为了保证线程安全，使用该属性后，线程要使用相应变量，必须先锁定mutex_
使得pendingFunctors_是原子操作。
    */
    Condition condition_ GUARDED_BY(mutex_);
    int count_ GUARDED_BY(mutex_);
};

}//muduo

#endif
```



### 测试程序

简单地验证程序是否能运行即可

```c++

#include "CountDownLatch.h"
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
    CountDownLatch cnt(3);
    return 0;
}
```


