### 类 FixedBuffer

缓冲区模板类，类中SIZE不是类型参数，而是缓冲区大小

#### 成员函数

* FixedBuffer()

 构造函数

* ~FixedBuffer()

析构函数

* append()

将新的字符串加入缓冲区(通过append()接口把日志内容添加到缓冲区来。

* data()

返回缓冲区的数据

* length_()

返回缓冲区中数据的长度

* current()

返回cur_

* avail() 

缓冲区还剩多少空间

* add(size_t len)

缓冲区数据增加了len

* reset() 

重置

* bzero()

清空缓冲区

* const char* debufString();

用作debug,其实就是输出缓冲区的内容

* setCookie()

设置cookie_函数

* string toString()

返回string类型的字符串

* StringPiece toStringPiece()

返回StringPiece类型的字符串

#### 静态函数

static void cookieStart();

static void cookieEnd();

在源代码LogStream.cc中函数体是空的

#### 成员变量

* char data_[SIZE]

缓冲区

* cur_;

缓冲区中的数据的最后一个字节的下一个字节

---

### 类 LogStream

muduo没有用到标准库的iostream，而是自己写的LogStream类，这主要是出于性能。

设计这个LogStream类，让它如同C++的标准输出流对象cout，能用<<符号接收输入，cout是输出到终端，

而LogStream类是把输出保存自己内部的缓冲区，可以让外部程序把缓冲区的内容重定向输出到不同的目标，

如文件、终端、socket

#### 成员函数

* LogStreamr & operator<<()

重载<<运算符。该类主要负责将要记录的日志内容放到这个Buffer里面。包括字符串，整型、double类型（整型和double要先将之转换成字符型，再放到buffer里面)。该类对这些类型都重载了<<操作符。这个LogStream类不做具体的IO操作。以后要是需要这个buffer里的数据，可以调用LogStream的buffer()函数，这个函数返回const Buffer&

* staticCheck()

检查不同类型最大值的位数是否合法，124-->3位

```c++
void LogStream::staticCheck(){
    static_assert(kMaxNumericSize - 10 > std::numeric_limits<double>::digits10,
                "kMaxNumericSize is large enough");
    static_assert(kMaxNumericSize - 10 > std::numeric_limits<long double>::digits10,
                "kMaxNumericSize is large enough");
    static_assert(kMaxNumericSize - 10 > std::numeric_limits<long>::digits10,
                "kMaxNumericSize is large enough");
    static_assert(kMaxNumericSize - 10 > std::numeric_limits<long long>::digits10,
                "kMaxNumericSize is large enough");
}
```

关于numeric_limits<T>::digits10 ，返回目标类型在十进制下可以表示的最大位数，值是类型 T 能无更改地表示的底 10 位数，即任何拥有这么多十进制有效数字的数能转换成 T 的值并转换回十进制形式，而不因舍入或上溢而更改。对于底 radix 类型，它是 digits （对于浮点类型是 digits-1 ）的值乘 log10(radix) 并向下取整。 

* formatInterger

通过模版函数formatInteger()把short、unsigned short、int、unsigned int、long、unsigned long、long long等类型转换为字符串，并保存到buffer中。

----

下面三个函数都是调用FixedBuffer中的函数来实现缓冲区的交互操作

* append() 

往缓冲区添加字符串

* buffer() 

返回缓冲区

* resetBuffer()

重设缓冲区

#### 成员变量

* buffer_

FixedBuffer类型的缓冲区



### 类Fmt

两个成员变量

char buf_[32];

int length_;

主要作用是在构造函数的时候就能进行静态断言

```c++
Fmt::Fmt(const char* fmt,T val){
    //断言是算术类型（即整数类型或浮点类型）
    static_assert(std::is_arithmetic<T::value==true,"Must be arithmetic type");
    ////按照fmt 格式将val 格式化成字符串放入buf_中
    length_=snprintf(buf_,sizeof(buf),fmt,val);
    assert(static_cast<size_t>(length_)<sizeof(buf_));
}
```



-----

#### 其他函数

* size_t convert(char buf[], T value)

将T（10进制整形）value转化成字符串

* size_t convertHex(char buf[],uintptr_t value)

将16进制的value转化成字符串

* formatSI(int64_t s)

根据s的大小生成以下格式的一行

```c++
 [0,     999]
 [1.00k, 999k]
 [1.00M, 999M]
 [1.00G, 999G]
 [1.00T, 999T]
 [1.00P, 999P]
 [1.00E, inf)
```

* formatIEC(int64_t s)

根据s的大小生成以下格式的一行

```c++
 [0, 1023]
 [1.00Ki, 9.99Ki]
 [10.0Ki, 99.9Ki]
 [ 100Ki, 1023Ki]
 [1.00Mi, 9.99Mi]
```



测试程序

网上博客关于LogStream的一个单元测试，略加修改后跑了以下，输出的是完成0~N的输入所花的时间

```c++

#include "LogStream.h"
#include "Timestamp.h"
 #include "Date.h"

#include <sstream>
#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

using namespace muduo;

const int N = 1000000;


void benchLogStream()
{
  Timestamp start=Timestamp::now();
  LogStream os;
  for (int i = 0; i < N; ++i)
  {
    os << (i);
    os.resetBuffer();
  }
  Timestamp end=Timestamp::now();
 
  printf("benchLogStream %f\n", timeDifference(end, start));
}
int main()
{
    benchLogStream();
    return 0;
}
```

运行结果:

```c++
knopfler@DESKTOP-3UDOCBE:~/muduo/base$ make
g++ -o test test.cpp LogStream.h LogStream.cpp Timestamp.h Timestamp.cpp
knopfler@DESKTOP-3UDOCBE:~/muduo/base$ ./test
benchLogStream 0.103164
```

