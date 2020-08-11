#### 工作流程

首先使用宏定义写日志

```c++
#define LOG_TRACE if (muduo::Logger::logLevel() <= muduo::Logger::TRACE) \
  muduo::Logger(__FILE__, __LINE__, muduo::Logger::TRACE, __func__).stream()
#define LOG_DEBUG if (muduo::Logger::logLevel() <= muduo::Logger::DEBUG) \
  muduo::Logger(__FILE__, __LINE__, muduo::Logger::DEBUG, __func__).stream()
#define LOG_INFO if (muduo::Logger::logLevel() <= muduo::Logger::INFO) \
  muduo::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN muduo::Logger(__FILE__, __LINE__, muduo::Logger::WARN).stream()
#define LOG_ERROR muduo::Logger(__FILE__, __LINE__, muduo::Logger::ERROR).stream()
#define LOG_FATAL muduo::Logger(__FILE__, __LINE__, muduo::Logger::FATAL).stream()
#define LOG_SYSERR muduo::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL muduo::Logger(__FILE__, __LINE__, true).stream()
```

举例来说，当我们使用 LOG_INFO << "info…" 时，

相当于用`__FILE__` (文件的完整路径和文件名)与 LINE(当前行号的整数 )初始化了一个Logger类，

出错时间、线程、文件、行号初始化号，接着将"info..."输入到缓冲区，在Logger生命周期结束时调用析构函数输出错误信息。

`if (muduo::Logger::logLevel() <= muduo::Logger::TRACE)`

这代表我们使用该LOG_INFO宏时会先进行判断，如果级别大于INFO级别，后面那句不会被执行，也就是不会打印INFO级别的信息。

整个调用过程：Logger  --> impl  --> LogStream  --> operator<<  FilxedBuffer --> g_output --> g_flush



#### 日志级别

TRACE

> 指出比DEBUG粒度更细的一些信息事件（开发过程中使用）

DEBUG

> 指出细粒度信息事件对调试应用程序是非常有帮助的。（开发过程中使用）

INFO

> 表明消息在粗粒度级别上突出强调应用程序的运行过程。

WARN

> 系统能正常运行，但可能会出现潜在错误的情形。

ERROR

> 指出虽然发生错误事件，但仍然不影响系统的继续运行。

FATAL

> 指出每个严重的错误事件将会导致应用程序的退出。

### Logger.h

```c++
#ifndef MUDUO_BASE_LOGGING_H
#define MUDUO_BASE_LOGGING_H

#include "LogStream.h"
#include "Timestamp.h"

namespace muduo{
class TimeZone;
class Logger{
public:
    /*
    级别类型
    muduo使用枚举的方式指出了输出错误信息的形式。
    其中TRACE,DEBUG用于调试，INFO,WARN,ERROR,FATAL在程序运行过程中输出，警告的程度依次上升，
    出现FATAL时程序将强制退出。NUM_LOG_LEVELS是这个枚举结构体中错误形式的数目，为6。
    */
    enum LogLevel{
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        NUM_LOG_LEVELS,
    };
    /*
    SourceFile类用于在输出信息的时候指明是在具体的出错文件。
    这个类不难，是一个简单的字符封装。它的私有成员有data_和size_。
    在Source的构造函数中，data_会被出错文件的路径初始化，然后经过strrchr处理，
    指向出错文件的basename(如/muduo/base/Thread.cc到Thread.cc)。size_就是basename的长度。
    */
    class SourceFile{
    public:
        template<int N>
        SourceFile(const char (&arr)[N])
            :data_(arr),size_(N-1){
            const char* slash = strrchr(data_, '/'); // builtin function
            if (slash)
            {
                data_ = slash + 1;
                size_ -= static_cast<int>(data_ - arr);
            }
        }
        explicit SourceFile(const char* filename)
            :data_(filename){
            const char* slash=strrchr(filename,'/');
            if(slash){
                data_=slash+1;
            }
            size_=static_cast<int>(strlen(data_));
        } 
        const char* data_;
        int size_;
    };

    Logger(SourceFile file, int line);
    Logger(SourceFile file, int line, LogLevel level);
    Logger(SourceFile file, int line, LogLevel level, const char* func);
    Logger(SourceFile file, int line, bool toAbort);
    ~Logger();

    LogStream& stream() { return impl_.stream_; }

    static LogLevel logLevel();//返回当前日志级别
    static void setLogLevel(LogLevel level);//设置日志级别

    typedef void (*OutputFunc)(const char* msg, int len);
    typedef void (*FlushFunc)();
    static void setOutput(OutputFunc);//设置输出函数，用来替代默认的
    static void setFlush(FlushFunc);//用来配套你设置的输出函数的刷新方法
    static void setTimeZone(const TimeZone& tz);
private:
    //Impl类由Logger调用，输出出错信息，封装了Logger的缓冲区stream_
    class Impl{
    public:
        typedef Logger::LogLevel LogLevel;
        Impl(LogLevel level,int old_errno,const SourceFile& file,int line);
        /*formatTime函数的作用是将出错的时间格式化问年-月-日-时-分-秒-毫秒的形式，
        并将格式化的时间字符串输入到stream_的缓冲区中。主要是利用了gmtime_r函数
        */
        void formatTime();
        void finish();

        Timestamp time_;//当前时间
        //构造日志缓冲区，该缓冲区重载了各种<<，都是将数据格式到LogStream的内部成员缓冲区buffer里
        LogStream stream_;
        
        LogLevel level_;//级别
        int line_;//行
        SourceFile basename_;//文件
    };

    Impl impl_;//logger构造这个对象
};
extern Logger::LogLevel g_logLevel;

//返回当前日志级别
inline Logger::LogLevel Logger::logLevel()
{
  return g_logLevel;
}


//
// CAUTION: do not write:
//
// if (good)
//   LOG_INFO << "Good news";
// else
//   LOG_WARN << "Bad news";
//
// this expends to
//
// if (good)
//   if (logging_INFO)
//     logInfoStream << "Good news";
//   else
//     logWarnStream << "Bad news";
//
/*
在Logger类中定义了一系列宏来实现错误输出。我们使用Logger来输出错误信息时，只需调用这些宏即可。
举例来说，当我们使用LOG_INFO << "info…"时，
相当于用__FILE__ (文件的完整路径和文件名)与 LINE(当前行号的整数 )初始化了一个Logger类，
出错时间、线程、文件、行号初始化号，接着将"infor"输入到缓冲区，在Logger生命周期结束时调用析构函数输出错误信息。
*/
//使用if条件判断，如果当前级别大于TRACE，就相当于没有下面一行代码，不会编译，下同。
#define LOG_TRACE if (muduo::Logger::logLevel() <= muduo::Logger::TRACE) \
  muduo::Logger(__FILE__, __LINE__, muduo::Logger::TRACE, __func__).stream()
#define LOG_DEBUG if (muduo::Logger::logLevel() <= muduo::Logger::DEBUG) \
  muduo::Logger(__FILE__, __LINE__, muduo::Logger::DEBUG, __func__).stream()
#define LOG_INFO if (muduo::Logger::logLevel() <= muduo::Logger::INFO) \
  muduo::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN muduo::Logger(__FILE__, __LINE__, muduo::Logger::WARN).stream()
#define LOG_ERROR muduo::Logger(__FILE__, __LINE__, muduo::Logger::ERROR).stream()
#define LOG_FATAL muduo::Logger(__FILE__, __LINE__, muduo::Logger::FATAL).stream()
#define LOG_SYSERR muduo::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL muduo::Logger(__FILE__, __LINE__, true).stream()

}

#endif
```



#### Logger.cpp

```c++
#include "Logging.h"

#include "CurrentThread.h"
#include "Timestamp.h"
#include "TimeZone.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sstream>

namespace muduo
{

__thread char t_errnobuf[512];
__thread char t_time[64];
__thread time_t t_lastSecond;

const char* strerror_tl(int savedErrno)
{
    return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
}
//设置日志级别
Logger::LogLevel initLogLevel()
{
    if (::getenv("MUDUO_LOG_TRACE")) //获取TRACE环境变量，如果有，返回它
        return Logger::TRACE;
    else if (::getenv("MUDUO_LOG_DEBUG"))//获取DEBUG环境变量，如果有，返回它
        return Logger::DEBUG;
    else
        return Logger::INFO;//如果它们都没有，就使用INFO级别
}

Logger::LogLevel g_logLevel = initLogLevel();

const char* LogLevelName[Logger::NUM_LOG_LEVELS] =
{
    "TRACE ",
    "DEBUG ",
    "INFO  ",
    "WARN  ",
    "ERROR ",
    "FATAL ",
};

// helper class for known string length at compile time
//编译时获取字符串长度的类
class T
{
 public:
    T(const char* str, unsigned len)
        :str_(str),
        len_(len)
    {
        assert(strlen(str) == len_);
    }

    const char* str_;
    const unsigned len_;
};

inline LogStream& operator<<(LogStream& s, T v)
{
    s.append(v.str_, v.len_);
    return s;
}

inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v)
{
    s.append(v.data_, v.size_);
    return s;
}
//默认输出内容到stdout
void defaultOutput(const char* msg, int len)
{
    size_t n = fwrite(msg, 1, len, stdout);
    //FIXME check n
    (void)n;
}
 //默认刷新方法
void defaultFlush()
{
    fflush(stdout);
}

Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush; 
TimeZone g_logTimeZone;
}//muduo

using namespace muduo;
/*
构造函数中，我们用现在的时间初始化time_,line_是出错处的文件行号，basename就是出错文件的文件名。
初始化接触后，执行formatTime函数，将当前线程号和LogLevel输入stream_的缓冲区。
`saveErrno我们一般传入errno(录系统的最后一次错误代码)，若不等于0，将它错误原因输入到缓冲区。
*/
//savedErrno 错误码，没有就传0
Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile& file, int line)
    : time_(Timestamp::now()),//当前时间
    stream_(),//初始化logger的四个成员
    level_(level),
    line_(line),
    basename_(file)
{
    formatTime();//格式化时间
    CurrentThread::tid();//缓存当前线程id
    //将当前线程号和LogLevel输入stream_的缓冲区
    stream_ << T(CurrentThread::tidString(), CurrentThread::tidStringLength());//格式化线程tid字符串
    stream_ << T(LogLevelName[level], 6);//格式化级别，对应成字符串，先输出到缓冲区
    //将它错误原因输入到缓冲区
    if (savedErrno != 0)
    {
        stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
    }
}
//格式化时间
void Logger::Impl::formatTime(){
    int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / Timestamp::kMicroSecondsPerSecond);
    int microseconds = static_cast<int>(microSecondsSinceEpoch % Timestamp::kMicroSecondsPerSecond);

    if (seconds != t_lastSecond)
    {
        t_lastSecond = seconds;
        struct tm tm_time;
        if (g_logTimeZone.valid())
        {
            tm_time = g_logTimeZone.toLocalTime(seconds);
        }
        else
        {
            /*gmtime_r() 就是将seconds转化成tm_time中保存的信息
            下面一条注释是源代码中的，不知道 FIXME 是代表什么，
            这个处理应该可以用TimeZone::fromUtcTime 来实现，
            */
            ::gmtime_r(&seconds, &tm_time); // FIXME TimeZone::fromUtcTime
        }

        int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
        tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
        tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        assert(len == 17); (void)len;
    }

    if (g_logTimeZone.valid())
    {
        Fmt us(".%06d ", microseconds);
        assert(us.length() == 8);
        stream_ << T(t_time, 17) << T(us.data(), 8); //用stream进行输出，重载了<<
    }
    else
    {
        Fmt us(".%06dZ ", microseconds);
        assert(us.length() == 9);
        stream_ << T(t_time, 17) << T(us.data(), 9); //用stream进行输出，重载了<<
    }
}

void Logger::Impl::finish()   //将名字，行号行输进缓冲区
{
    stream_ << " - " << basename_ << ':' << line_ << '\n';
}

Logger::Logger(SourceFile file, int line)
    : impl_(INFO, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
  : impl_(level, 0, file, line)
{
    impl_.stream_ << func << ' ';//输出格式化函数名称
}

Logger::Logger(SourceFile file, int line, LogLevel level)
    : impl_(level, 0, file, line)
{
}
/*
toAbort 是否终止，FATAL这一日志级别会导致应用程序的退出
*/
Logger::Logger(SourceFile file, int line, bool toAbort)
    : impl_(toAbort?FATAL:ERROR, errno, file, line)
{
}
//析构函数中会调用impl_的finish方法
Logger::~Logger()
{
    impl_.finish();//将名字，行数输入缓冲区
    const LogStream::Buffer& buf(stream().buffer());//将缓冲区以引用方式获得
    g_output(buf.data(), buf.length());//调用全部输出方法，输出缓冲区内容，默认是输出到stdout
    if (impl_.level_ == FATAL)
    {
        g_flush();
        abort();
    }
}

void Logger::setLogLevel(Logger::LogLevel level)//设置日志级别
{
    g_logLevel = level;
}

void Logger::setOutput(OutputFunc out)//设置输出函数，用来替代默认的
{
    g_output = out;
}

void Logger::setFlush(FlushFunc flush)//用来配套你设置的输出函数的刷新方法
{
    g_flush = flush;
}

void Logger::setTimeZone(const TimeZone& tz)
{
    g_logTimeZone = tz;
}
```


