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