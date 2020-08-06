基本类型的声明，包括muduo::string

类型转换 implicit_cast 和 down_cast

---

### 1. 基本类型的声明

```c++
using std::string;
//简化了用 memset 初始化的使用
inline void memZero(void* p,size_t n){
    memset(p,0,n);
}
```

----

**隐式类型转换**：隐式类型转化是编译器默默地、隐式地、偷偷地进行的类型转换，这种转换不需要程序员干预，会自动发生,比如赋值转换 `float f = 100;`

**强制类型转换**: C++中有四种强制转换 const_cast，static_cast，dynamic_cast，reinterpret_cast



在类层次结构中基类（父类）和派生类（子类）之间指针或引用的转换时，

进行**上行转换**（把派生类的指针或引用转换成基类表示）是安全的；

进行**下行转换**（把基类指针或引用转换成派生类表示）时，由于没有动态类型检查，所以是不安全的。

### 2.1 implicit_cast 隐式转换

```c++
template<typename To,typename From>
inline To implicit_cast(From const &f){
    return f;
}
```

implicit_cast 是一个简单的隐式转换。

原型:

`implicit_cast<ToType>(expr)`

用法：

```c++
int i = 3;
implicit_cast<double>(i);
```

用处:

用于在继承关系中， 子类指针转化为父类指针；隐式转换inferred 推断，因为 from type 可以被推断出来，所以使用方法和 static_cast 相同在up_cast时应该使用implicit_cast替换static_cast,因为前者比后者要安全。

以一个**例子**说明,在菱形继承中，static_cast把最底层的对象可以转换为中层对象，这样编译可以通过，但是在运行时可能崩溃

implicit_cast就不会有这个问题，在编译时就会给出错误信息

### 2.2 down_cast 向下转换

```c++
template<typename To,typename From>
inline To down_cast(From const *f){
 
#if !defined(NDEBUG) && !defined(GOOGLE_PROTOBUF_NO_RTTI)
    assert(f == NULL || dynamic_cast<To>(f) != NULL);  // RTTI: debug mode only!
#endif
    
    return static_cast<To>(f);
}
```

down_cast在debug模式下内部使用了dynamic_cast进行验证，在release下使用static_cast替换dynamic_cast。

  为什么使用down_cast而不直接使用dynamic_cast?

  1.因为但凡程序设计正确，dynamic_cast就可以用static_cast来替换，而后者比前者更有效率。

  2.dynamic_cast可能失败(在运行时crash)，运行时RTTI不是好的设计，不应该在运行时RTTI或者需要RTTI时一般都有更好的选择。

-----

对于两个转换的测试

```c++
#include "Types.h"

#include<iostream>
#include<stdio.h>
using namespace muduo;
class Top {};
class MiddleA :public Top {};
class MiddleB :public Top {};
class Bottom :public MiddleA, public MiddleB {};

void Function(MiddleA& A)
{
    printf("MiddleA Function\n");
}
void Function(MiddleB& B)
{
    printf("MiddleB Function\n");
}

int main()
{
    Bottom bottom;
    Top top;
    //Function(static_cast<MiddleB&>(top));//编译可以通过，但是在运行时可能崩溃，因为top不是“一种”MiddleB，但是static_cast不能发现这个问题
    //Function(implicit_cast<MiddleA, Top>(top));//编译不通过，在编译时就会给出错误信息
    MiddleA ma=implicit_cast<MiddleA, Bottom>(bottom);//成功运行
    Function(ma);
    
    //Function(bottom);//编译不通过,bottom既可以默认转换为MiddleA，也可以默认转换为MiddleB，如果不明确指出就会出现歧义
    //Function(static_cast<MiddleA&>(bottom));//OK

    //MiddleB *pBottom = down_cast<MiddleB *>(&top);//编译不通过，dynamic_cast无法转化
    return 0;
}
```

