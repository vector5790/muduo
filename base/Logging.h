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