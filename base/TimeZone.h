/*
时区与夏令时
*/
#ifndef MUDUO_BASE_TIMEZONE_H
#define MUDUO_BASE_TIMEZONE_H

#include "copyable.h"
#include <memory>
#include <time.h>

namespace muduo{
// TimeZone for 1970~2030
class TimeZone :public muduo::copyable{
public:
    //构造函数是explicit的，内置指针不能隐式的转换，只能使用初始化的形式
    explicit TimeZone(const char* zonefile);
    TimeZone(int eastOfUtc,const char* tzname);
    TimeZone()=default;

    bool valid()const {
        return static_cast<bool>(data_);
    }

    struct tm toLocalTime(time_t secondsSinceEpoth) const;
    time_t fromLocalTime(const struct tm&) const;

    static struct tm toUtcTime(time_t secondsSinceEpoch,bool yday=false);
    static time_t fromUtcTime(const struct tm&);
    static time_t fromUtcTime(int year,int month,int day,int hour,int minute,int seconds);

    struct Data;
private:
    std::shared_ptr<Data>data_;
};

}

#endif