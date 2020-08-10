C++里面有string和char*，如果你用const string &s 做函数形参，可以同时兼容两种字符串。但当你传入一个很长的char * 时，char*转string，创建一个临时string对象,同时开辟一个内存空间，拷贝字符串, 开销比较大。如果你的目的仅仅是读取字符串的值，用这个StringPiece的话，仅仅是4+一个指针的内存开销，而且也保证了兼容性。所以这个类的目的是传入字符串的字面值，它内部的ptr_ 这块内存不归他所有。所以不能做任何改动。归根结底，是处于性能的考虑，用以实现高效的字符串传递，这里既可以用const char*，也可以用std::string类型作为参数，并且不涉及内存拷贝。
实际上，这个类是google提供的一个类。

### 成员函数

* data()

返回该字符串

* size() 

返回字符串的长度

* empty() 

判断字符串是否为空

* begin() 

返回字符串的开头

* end() 

返回字符串的最后一个字节的下一位置

* clear() 

清空

* void set()

将字符串指针ptr_指向新的字符串的地址

operator[] (int i) 

重载[],使该类能像普通字符串按下标取值

* remove_prefix(int n)

删除字符串前面n位

* remove_suffix(int n)

删除字符串后面n位

### 成员变量

ptr_

字符串指针，指向传入的字符串地址

length_

长度

---



重载了< 、<=、 >= 、>这些运算符。这些运算符实现起来大同小异，所以通过一个宏STRINGPIECE_BINARY_PREDICATE来实现

STRINGPIECE_BINARY_PREDICATE有两个参数，第二个是辅助比较运算符

比如”abcd” < “abcdefg”, memcp比较它们的前四个字节，得到的r的值是0，

很明显”adbcd”是小于”abcdefg”但由return后面的运算返回的结果为true。

又比如”abcdx” < “abcdefg”, memcp比较它们的前5个字节，r的值为大于0，

显然，((r < 0) || ((r == 0) && (length_ < x.length_)))得到的结果为false.

```c++
#define STRINGPIECE_BINARY_PREDICATE(cmp,anxcmp) \

  bool operator cmp(const StringPiece& x)const{\

​    int r=memcmp(ptr_,x.ptr_,length_<x.length_?length_:x.length_);\

​    return ((r anxcmp 0)||((r==0)&&(length_ cmp x.length_)));\

  }

  STRINGPIECE_BINARY_PREDICATE(<, <);

  STRINGPIECE_BINARY_PREDICATE(<=, <);

  STRINGPIECE_BINARY_PREDICATE(>=, >);

  STRINGPIECE_BINARY_PREDICATE(>, >);

#undef STRINGPIECE_BINARY_PREDICATE

```

