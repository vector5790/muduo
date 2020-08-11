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