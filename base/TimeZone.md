作用：时区与夏令时,方便时区之间的转换，以及时令之间的转换

前置知识：先阅读过Data，noncopyable，copyable 系列源代码

```c++
#include <time.h>
struct tm {
    int tm_sec;       /* 秒 – 取值区间为[0,59] */
    int tm_min;       /* 分 - 取值区间为[0,59] */
    int tm_hour;      /* 时 - 取值区间为[0,23] */
    int tm_mday;      /* 一个月中的日期 - 取值区间为[1,31] */
    int tm_mon;     /* 月份（从一月开始，0代表一月） 取值区间为[0,11] */
    int tm_year;      /* 年份，其值等于实际年份减去1900 */
    int tm_wday;      /* 星期 – 取值区间为[0,6]，其中0代表星期天，1代表星期一，以此类推 */
          int tm_yday;      /* 从每年的1月1日开始的天数 – 取值区间为[0,365]，其中0代表1月1日，1代表1月2日，以此类推 */
          int tm_isdst;     /* 夏令时标识符，实行夏令时的时候，tm_isdst为正。不实行夏令时的进候，tm_isdst为0；不了解情况时，tm_isdst()为负。*/
};
```

```c++
time_t表示的时间（日历时间）是从一个时间点（例如：1970年1月1日0时0分0秒）到此时的秒数，它是一个长整数，
```



### 成员函数

* TimeZone(const char* zonefile)

构造函数,构造函数是explicit的，内置指针不能隐式的转换，只能使用初始化的形式

关于zonefile，应该是保存时区信息的一个文件的地址，通过读取该文件中的信息得出时区，但我并不知道该文件文件是什么类型的，网上也搜不到，故暂且搁置理解这种构造方式，待日后知晓该文件类型后在进行理解。



所以该构造函数所用到的其他函数，类都留到日后理解，如class File，readTimeZoneFile()。特别的，对于函数findLocaltime(),只考虑以下的if语句中的代码即可，else语句中的是TimeZone(const char* zonefile)该种构造方式需要用到的

```c++
if(data.transitions.empty()||comp(sentry,data.transitions.front())){
        local=&data.localtimes.front();
}
```



* TimeZone(int eastOfUtc, const char* tzname);

eastOfUtc表示UTC时间。中国内地的时间与UTC的时差为+8，也就是UTC+8

这里根据源代码，eastOfUtc 应该是秒数

* TimeZone() = default

默认构造函数

* toLocalTime(time_t secondsSinceEpoch)

UTC时间转换成当地时间

* toUtcTime(time_t secondsSinceEpoch, bool yday = false)

当地时间转换成UTC时间

fromUtcTime()

返回从1970-1-1到现在的UTC时间经过多少秒

* fromLocalTime(const struct tm&)  

返回从1970-1-1到现在的当地时间经过多少秒



### 成员变量

* shared_ptr<Data> data_



---





测试代码

```c++
#include <muduo/base/TimeZone.h>
#include <muduo/base/Timestamp.h>
#include <muduo/base/Date.h>
 
#include <time.h>
 
#include <iostream>
using namespace muduo;
void PrintTm(struct tm& T)
{
    std::cout<<T.tm_year<<"-"<<T.tm_mon<<"-"<<T.tm_mday<<std::endl;
    std::cout<<T.tm_hour<<"-"<<T.tm_min<<"-"<<T.tm_sec<<std::endl;
    std::cout<<T.tm_wday<<std::endl;
}
int main()
{
    
    Timestamp timeStamp=Timestamp::now();
    struct tm T=TimeZone::toUtcTime(timeStamp.secondsSinceEpoch());//通过类调用静态成员函数
    PrintTm(T);//这个打印的月份要+1才是正确的月份时间，见Date.cc中的构造函数。
    Date dt(T);
    std::cout<<dt.toIsoString()<<std::endl;//显示正确的年月日
    //TimeZone timeZone(8, "China");//作者希望传入的是偏移的秒数，而不是偏移的时区数
    TimeZone timeZone(8*60*60, "China");
    struct  tm T2=timeZone.toLocalTime(timeStamp.secondsSinceEpoch());
    PrintTm(T2);//这个能显示正确的时分秒
    
    return 0;
}
```

