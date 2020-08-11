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