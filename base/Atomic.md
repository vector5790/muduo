### 类 AtomicInteger

封装了原子操作，目的是在使用中统计消息数目。Atomic的私有成员只有一个value_。注意修饰词volatile说明编译器每次使用value_时必须从内存里重新读取，不能使用寄存器中的备份。

而它的公有成员均是对这个变量进行原子性操作的成员函数，这些函数主要就是封装了`__sync_`系列的函数调用

### Atomic源码注释

```c++
/*
原子操作与原子整数
*/
#ifndef MUDUO_BASE_ATOMIC_H
#define MUDUO_BASE_ATOMIC_H

#include "noncopyable.h"
#include <stdint.h>

namespace muduo{

namespace detail{

template<typename T>
class AtomicIntegerT:noncopyable{
public:
    AtomicIntegerT():value_(0){}

    T get(){
        /*
        type __sync_val_compare_and_swap (type *ptr, type oldval type newval, ...)
        提供原子的比较和交换，如果*ptr == oldval,就将newval写入*ptr,
        返回操作之前的值。
        */
        return __sync_val_compare_and_swap(&value_,0,0);
    }
    T getAndAdd(T x){
        /*
        先fetch，然后加x，返回的是加x以前的值
        */
        return __sync_fetch_and_add(&value_,x);
    }
    T addAndGet(T x){
        /*
        先加x，再返回的现在的值
        */
        return getAndAdd(x)+x;
    }
    T incrementAndGet(){
        /*
        相当于 ++i
        */
        return addAndGet(1);
    }
    T decrementAndGet(){
        /*
        相当于 --i
        */
        return addAndGet(-1);
    }
    void add(T x){
        /*
        加x
        */
        getAndAdd(x);
    }
    void increment(){
        /*
        自增1
        */
        incrementAndGet();
    }
    void decrement(){
        /*
        自减1
        */
        decrementAndGet();
    }
    T getAndSet(T newValue){
        /*
        type __sync_lock_test_and_set (type *ptr, type value, ...)
        将*ptr设为value并返回*ptr操作之前的值
        */
        return __sync_lock_test_and_set(&value_,newValue);
    }

private:
    volatile T value_;
};
}

typedef detail::AtomicIntegerT<int32_t> AtomicInt32;
typedef detail::AtomicIntegerT<int64_t> AtomicInt64;

}
#endif
```



---

测试代码

```c++
#include "Atomic.h"
#include <sstream>
#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <iostream>
using namespace muduo;

const int N = 10;

int main()
{
    AtomicInt32 A;
    std::cout<<A.get()<<std::endl;
    A.increment();
    std::cout<<"After increment: "<<A.get()<<std::endl;
    std::cout<<"After add 5: "<<A.addAndGet(5)<<std::endl;
    return 0;
}
```

测试结果:

```c++
knopfler@DESKTOP-3UDOCBE:~/muduo/base$ make
g++ -o test test.cpp Mutex.h Condition.h Condition.cpp CurrentThread.h CurrentThread.cpp -lpthread CountDownLatch.h CountDownLatch.cpp Atomic.h
knopfler@DESKTOP-3UDOCBE:~/muduo/base$ ./test
0
After increment: 1
After add 5: 6
```

