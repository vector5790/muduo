#ifndef MUDUO_BASE_TYPES_H
#define MUDUO_BASE_TYPES_H

#include <stdint.h>
#include <string.h>  // memset
#include <string>

#ifndef NDEBUG
#include <assert.h>
#endif

namespace muduo{

using std::string;
//简化了用 memset 初始化的使用
inline void memZero(void* p,size_t n){
    memset(p,0,n);
}
/* 
implicit_cast is the same as for static_cast etc.:
implicit_cast<ToType>(expr)

用于在继承关系中， 子类指针转化为父类指针；隐式转换
inferred 推断，因为 from type 可以被推断出来，所以使用方法和 static_cast 相同
在up_cast时应该使用implicit_cast替换static_cast,因为前者比后者要安全。
以一个例子说明,在菱形继承中，static_cast把最底层的对象可以转换为中层对象，这样编译可以通过，但是在运行时可能崩溃
implicit_cast就不会有这个问题，在编译时就会给出错误信息
*/
template<typename To,typename From>
inline To implicit_cast(From const &f){
    return f;
}
// When you upcast (that is, cast a pointer from type Foo to type
// SuperclassOfFoo), it's fine to use implicit_cast<>, since upcasts
// always succeed.  When you downcast (that is, cast a pointer from
// type Foo to type SubclassOfFoo), static_cast<> isn't safe, because
// how do you know the pointer is really of type SubclassOfFoo?  It
// could be a bare Foo, or of type DifferentSubclassOfFoo.  Thus,
// when you downcast, you should use this macro.  In debug mode, we
// use dynamic_cast<> to double-check the downcast is legal (we die
// if it's not).  In normal mode, we do the efficient static_cast<>
// instead.  Thus, it's important to test in debug mode to make sure
// the cast is legal!
//    This is the only place in the code we should use dynamic_cast<>.
// In particular, you SHOULDN'T be using dynamic_cast<> in order to
// do RTTI (eg code like this:
//    if (dynamic_cast<Subclass1>(foo)) HandleASubclass1Object(foo);
//    if (dynamic_cast<Subclass2>(foo)) HandleASubclass2Object(foo);
// You should design the code some other way not to need this.
template<typename To,typename From>
inline To down_cast(From const *f){// so we only accept pointers
    /*
    down_cast在debug模式下内部使用了dynamic_cast进行验证，在release下使用static_cast替换dynamic_cast。
    为什么使用down_cast而不直接使用dynamic_cast?
    1.因为但凡程序设计正确，dynamic_cast就可以用static_cast来替换，而后者比前者更有效率。
    2.dynamic_cast可能失败(在运行时crash)，运行时RTTI不是好的设计，不应该在运行时RTTI或者需要RTTI时一般都有更好的选择。
    */
#if !defined(NDEBUG) && !defined(GOOGLE_PROTOBUF_NO_RTTI)
    assert(f == NULL || dynamic_cast<To>(f) != NULL);  // RTTI: debug mode only!
#endif
    return static_cast<To>(f);
}

}

#endif