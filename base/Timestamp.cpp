#include "./Timestamp.h"

#include <sys/time.h>
#include <stdio.h>
/*
C++使用PRID64，需要两步：
包含头文件：<inttypes.h>
定义宏：__STDC_FORMAT_MACROS，可以通过编译时加-D__STDC_FORMAT_MACROS，或者在包含文件之前定义这个宏。
*/
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif  

#include <inttypes.h>

using namespace muduo;

static_assert(sizeof(Timestamp) == sizeof(int64_t),
              "Timestamp is same size as int64_t");

string Timestamp::toString()const{
    char buf[32]={0};
    int64_t seconds=microSecondsSinceEpoch_/kMicroSecondPerSecond;
    int64_t microseconds=microSecondsSinceEpoch_%kMicroSecondPerSecond;
    /*
    seconds:123
    microseconds:456
    => buf: 123.000456
    */
    snprintf(buf,sizeof(buf),"%" PRId64 ".%06" PRId64 "",seconds,microseconds);
    return buf;
}

string Timestamp::toFormattedString(bool showMicroseconds)const{
    char buf[64]={0};
    time_t seconds=static_cast<time_t>(microSecondsSinceEpoch_/kMicroSecondPerSecond);

    struct tm tm_time;
    //（线程安全的）// 把time_t结构中的信息转换成真实世界所使用的时间日期，存储在tm_time结构中
    gmtime_r(&seconds,&tm_time);

    if(showMicroseconds){
        int microseconds=static_cast<int>(microSecondsSinceEpoch_%kMicroSecondPerSecond);
         // 格式化输出时间戳    
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
             microseconds);
    }
    else{
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    }    
    
    return buf;
}

Timestamp Timestamp::now(){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    int64_t seconds = tv.tv_sec;
    return Timestamp(seconds * kMicroSecondPerSecond+tv.tv_usec)
}