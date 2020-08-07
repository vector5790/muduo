作用：UTC 时间戳

前置知识：先阅读过Types，copyable 系列源代码

### 常量

* kMicroSecondPerSecond

每秒所对应的微秒数

### 成员函数

* **swap()**

交换函数

* **toString()** 

将时间转换为string类型

```c++
string Timestamp::toString()const{
    char buf[32]={0};
    int64_t seconds=microSecondsSinceEpoch_/kMicroSecondPerSecond;
    int64_t microseconds=microSecondsSinceEpoch_%kMicroSecondPerSecond;
    snprintf(buf,sizeof(buf),"%" PRId64 ".%06" PRId64 "",seconds,microseconds);
    return buf;
}
```

将microSecondsSinceEpoch_转化为秒数和微秒数的组合，然后使用snprintf转成一个符合格式的字符串。

```C++
一个例子
seconds:123
microseconds:456
=> buf: 123.000456
```
这里PRId64是一种跨平台的书写方式，主要是为了同时支持32位和64位操作系统。PRId64表示64位整数，在32位系统中表示long long int，在64位系统中表示long int。相当于：

```c++
printf("%" "ld" "\n", value);  //64bit OS
printf("%" "lld" "\n", value);  //32bit OS
```

C++使用PRID64，需要两步：

包含头文件：<inttypes.h>

定义宏：__STDC_FORMAT_MACROS，可以通过编译时加-D__STDC_FORMAT_MACROS，或者在包含文件之前定义这个宏。

* **toFormattedString()** 

将时间转换为固定格式的string类型

将microSecondsSinceEpoch_转化为年月日分秒的字符串形式。这个功能借助于struct tm和gmtime函数。

tm结构体定义如下:

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
string Timestamp::toFormattedString(bool showMicroseconds)const{
    char buf[64]={0};
    /*time_t表示的时间（日历时间）是从一个时间点（例如：1970年1月1日0时0分0秒）到此时的秒数，它是一个长整数*/
    time_t seconds=static_cast<time_t>(microSecondsSinceEpoch_/kMicroSecondPerSecond);

    struct tm tm_time;
    //（线程安全的）把time_t结构中的信息转换成真实世界所使用的时间日期，存储在tm_time结构中
    gmtime_r(&seconds,&tm_time);
	//如果要输出微妙
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
```

* valid()

判断Timestamp是否有效

* microSecondsSinceEpoch()/secondsSinceEpoth()

返回1970-01-01 00:00:00 UTC的微秒数/秒数

* now()

返回当前时间的Timestamp

* invalid()

返回一个无效的Timestamp



  static Timestamp fromUnixTime(time_t t)

  static Timestamp fromUnixTime(



### 成员变量

* microSecondsSinceEpoch_

表示到1970-01-01 00:00:00 UTC的微秒数



测试代码

```c++
#include <muduo/base/TimeZone.h>
#include <muduo/base/Timestamp.h>

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
    struct tm T=TimeZone::toUtcTime(timeStamp.secondsSinceEpoch());
    PrintTm(T);

    TimeZone timeZone(8, "China");

    struct  tm T2=timeZone.toLocalTime(timeStamp.secondsSinceEpoch());
    PrintTm(T2);

    return 0;
}
```

----

**其他知识点**

1)

Timestamp类继承自boost::less_than_comparable <T>模板类

只要实现 <，即可自动实现>,<=,>=

```c++
class Timestamp : public muduo::copyable,
                  public boost::equality_comparable<Timestamp>,
                  public boost::less_than_comparable<Timestamp>{
};
```

2)

编译时断言，让错误发生在编译时，（assert是运行时断言）

```c++
static_assert(sizeof(Timestamp) == sizeof(int64_t),
              "Timestamp is same size as int64_t");
```

------

### 测试程序

```c++
#include "Timestamp.h"

#include<iostream>
#include<stdio.h>
using namespace muduo;

int main()
{
    Timestamp t=Timestamp::now();
    std::cout<<t.toString()<<std::endl;
    std::cout<<t.toFormattedString()<<std::endl;

    return 0;
}
```

测试结果

```c++
knopfler@DESKTOP-3UDOCBE:~/muduo/base$ make
g++ -o test test.cpp Timestamp.h Timestamp.cpp 
knopfler@DESKTOP-3UDOCBE:~/muduo/base$ ./test
1596784852.541282
20200807 07:20:52.541282
```

