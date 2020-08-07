作用：julian日历库(即公历)



### 成员结构体

* YearMonthDay

  struct YearMonthDay
  {
   int year; // [1900..2500]
    int month;  // [1..12]
    int day;  // [1..31]
  };

该结构体表示通用的日期格式

### 成员函数

* Date()

构造函数

* Date(int year, int month, int day);

用年月日初始化 julianDayNumber_

* Date(int julianDayNum)

用儒略日数初始化 julianDayNumber_

* Date(const struct tm&);

用结构体tm初始化 julianDayNumber_

* swap(Date& that)

交换两个对象的 julianDayNumber_

* valid()

julianDayNumber_ 是否有效

* toIsoString()

将当前日期转化为以下格式:

YYYY-MM-DD

* yearMonthDay()

调用 getYearMonthDay 将儒略日转化为通用的日期格式

* year()

返回当前的年份

* month()

返回当前的月份

* day()

返回当前的日期

* weekDay

返回当前是星期几

### 成员变量

* kDaysPerWeek 

一星期7天

* kJulianDayOf1970_01_01

1970年1月1日的儒略日数

* julianDayNumber_

儒略日数

---



Data.cpp定义的其他函数

* int getJulianDayNumber(int year, int month, int day)

将通用的日期格式转化为儒略日

* struct YearMonthDay getYearMonthDay(int julianDayNumber)

将儒略日转化为通用的日期格式

---

测试程序

```c++
#include "Date.h"

#include<iostream>
#include<stdio.h>
using namespace muduo;

int main()
{
    Date t(2020,8,7);
    std::cout<<t.toIsoString()<<std::endl;
    std::cout<<t.julianDayNumber()<<std::endl;

    return 0;
}
```

输出结果

```c++
knopfler@DESKTOP-3UDOCBE:~/muduo/base$ make
g++ -o test test.cpp Date.h Date.cpp 
knopfler@DESKTOP-3UDOCBE:~/muduo/base$ ./test
2020-08-07
2459069
```

