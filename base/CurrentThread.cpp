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