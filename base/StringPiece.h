/*
从Google开源代码借用的字符串参数传递类型
*/
#ifndef MUDUO_BASE_STRINGPIECE_H
#define MUDUO_BASE_STRINGPIECE_H

#include <string.h>
#include <iosfwd>    // for ostream forward-declaration

#include "Types.h"

namespace muduo{
// For passing C-style string argument to a function.
class StringArg{// copyable
public:
    StringArg(const char* str)
        :str_(str)
    {}
    StringArg(const string& str)
        :str_(str.c_str())
    {}

    const char* c_str() const { return str_; }

private:
    const char* str_;
};
class StringPiece{
private:
    const char* ptr_;
    int length_;
public:
    StringPiece()
        : ptr_(NULL),length_(0){ }
    StringPiece(const char* str)
        : ptr_(str),length_(static_cast<int>(strlen(ptr_))){ }
    StringPiece(const unsigned char* str)
        : ptr_(reinterpret_cast<const char*>(str)),length_(static_cast<int>(strlen(ptr_))){ }
    StringPiece(const char* offset,int len)
        : ptr_(offset),length_(len) { }
    
    const char* data()const { return ptr_; }
    int size() const { return length_; }
    bool empty() const { return length_==0; }
    const char* begin() const { return ptr_; }
    const char* end() const { return ptr_+length_; }

    void clear() { ptr_=NULL; length_=0; }
    void set(const char * buffer,int len) { ptr_ = buffer; length_ = len; }
    void set(const char* buffer){
        ptr_=buffer;
        length_=static_cast<int>(strlen(buffer));
    }
    void set(const void* buffer,int len){
        ptr_=reinterpret_cast<const char*>(buffer);
        length_=len;
    }

    char operator[] (int i) const { return ptr_[i]; }

    void remove_prefix(int n){
        ptr_+=n;
        length_-=n;
    }
    void remove_suffix(int n){
        length_-=n;
    }

    bool operator ==(const StringPiece& x)const{
        return ((length_==x.length_)&&(memcmp(ptr_,x.ptr_,length_)==0));
    }
    bool operator !=(const StringPiece& x)const{
        return !(*this==x);
    }
/*
重载了< 、<=、 >= 、>这些运算符。这些运算符实现起来大同小异，所以通过一个宏STRINGPIECE_BINARY_PREDICATE来实现
STRINGPIECE_BINARY_PREDICATE有两个参数，第二个是辅助比较运算符
比如”abcd” < “abcdefg”, memcp比较它们的前四个字节，得到的r的值是0，
很明显”adbcd”是小于”abcdefg”但由return后面的运算返回的结果为true。
又比如”abcdx” < “abcdefg”, memcp比较它们的前5个字节，r的值为大于0，
显然，((r < 0) || ((r == 0) && (length_ < x.length_)))得到的结果为false.
*/
#define STRINGPIECE_BINARY_PREDICATE(cmp,anxcmp) \
    bool operator cmp(const StringPiece& x)const{\
        int r=memcmp(ptr_,x.ptr_,length_<x.length_?length_:x.length_);\
        return ((r anxcmp 0)||((r==0)&&(length_ cmp x.length_)));\
    }
    STRINGPIECE_BINARY_PREDICATE(<,  <);
    STRINGPIECE_BINARY_PREDICATE(<=, <);
    STRINGPIECE_BINARY_PREDICATE(>=, >);
    STRINGPIECE_BINARY_PREDICATE(>,  >);
#undef STRINGPIECE_BINARY_PREDICATE

    int compare(const StringPiece& x) const {
        int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_);
        if (r == 0) {
            if (length_ < x.length_) r = -1;
            else if (length_ > x.length_) r = +1;
        }
        return r;
    }
    string as_string() const {
        return string(data(), size());
    }
    /*
    string& assign ( const char* s, size_t n );
    将字符数组或者字符串的首n个字符替换原字符串内容
    */
    void CopyToString(string* target) const {
        target->assign(ptr_, length_);
    }

  // Does "this" start with "x"
    bool starts_with(const StringPiece& x) const {
        return ((length_ >= x.length_) && (memcmp(ptr_, x.ptr_, x.length_) == 0));
    }
};

}//muduo


// allow StringPiece to be logged
std::ostream& operator<<(std::ostream& o, const muduo::StringPiece& piece);
#endif