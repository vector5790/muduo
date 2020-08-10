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