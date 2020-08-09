/*
muduo没有用到标准库的iostream，而是自己写的LogStream类，这主要是出于性能。

设计这个LogStream类，让它如同C++的标准输出流对象cout，能用<<符号接收输入，cout是输出到终端，
而LogStream类是把输出保存自己内部的缓冲区，可以让外部程序把缓冲区的内容重定向输出到不同的目标，
如文件、终端、socket
*/
#ifndef MUDUO_BASE_LOGSTREAM_H
#define MUDUO_BASE_LOGSTREAM_H

#include "noncopyable.h"
#include "StringPiece.h"
#include "Types.h"
#include <assert.h>
#include <string.h> // memcpy

namespace muduo{
namespace detail{

const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000*1000;

//缓冲区模板类，类中SIZE不是类型参数，而是缓冲区大小
template<int SIZE> 
class FixedBuffer : noncopyable{
public:
    FixedBuffer()
        :cur_(data_){
        setCookie(cookieStart);
    }
    ~FixedBuffer(){
        setCookie(cookieEnd);
    }
    void append(const char* buf,size_t len){
        //缓冲区放得下
        if(implicit_cast<size_t>(avail())>len){
            memcpy(cur_,buf,len);
            cur_+=len;
        }
    }

    const char* data() const { return data_; }
    int length() const { return static_cast<int>(cur_-data_);}

    // write to data_ directly
    char* current() { return cur_; }
    //缓冲区还剩多少空间
    int avail() const { return static_cast<int>(end()-cur_); }
    void add(size_t len) { cur_+=len; }
    void reset() {cur_ = data_; }
    void bzero() {memZero(data_,sizeof data_); }

    //for used by GDB
    const char* debugString();
    void setCookie(void (*cookie)()) {cookie_=cookie; }

    //for used by unit test
    string toString() const { return string(data_,length());}
    StringPiece toStringPiece() const { return StringPiece(data_,length()); }

private:
    const char* end() const { return data_+sizeof data_;};

    static void cookieStart();
    static void cookieEnd();

    void (*cookie_)();
    char data_[SIZE];
    char* cur_;
};

}//detail

class LogStream : noncopyable{
    typedef LogStream self;
public:
    typedef detail::FixedBuffer<detail::kSmallBuffer> Buffer;
    self& operator<<(bool v){
        buffer_.append(v?"1":"0",1);
        return *this;
    }

    self& operator<<(short);
    self& operator<<(unsigned short);
    self& operator<<(int);
    self& operator<<(unsigned int);
    self& operator<<(long);
    self& operator<<(unsigned long);
    self& operator<<(long long);
    self& operator<<(unsigned long long);

    self& operator<<(const void*);

    self& operator<<(float v){
        *this<<static_cast<double>(v);
        return *this;
    }
    self& operator<<(double);

    self& operator<<(char v){
        buffer_.append(&v,1);
        return *this;
    }

    self& operator<<(const char* str){
        if(str){
            buffer_.append(str,strlen(str));
        }
        else{
            buffer_.append("(null)",6);
        }
        return *this;
    }
    
    self& operator<<(const unsigned char* str){
        return operator<<(reinterpret_cast<const char*>(str));
    }

    self& operator<<(const string& v){
        buffer_.append(v.c_str(),v.size());
        return *this;
    }
    
    self& operator<<(const StringPiece& v){
        buffer_.append(v.data(),v.size());
        return *this;
    }

    self& operator<<(const Buffer& v){
        *this<<v.toStringPiece();
        return *this;
    }

    void append(const char* data,int len) { buffer_.append(data,len); }
    const Buffer& buffer() const { return buffer_; }
    void resetBuffer() { buffer_.reset(); }

private:
    //检查不同类型最大值的位数是否合法，124-->3位
    void staticCheck();

    template<typename T>
    void formatInteger(T);

    Buffer buffer_;

    static const int kMaxNumericSize =32;
};

class Fmt{
public:
    template<typename T>
    Fmt(const char* fmt,T val);

    const char* data() const { return buf_; }
    int length() const { return length_; }
private:
    char buf_[32];
    int length_;
};

inline LogStream& operator<<(LogStream& s,const Fmt& fmt){
    s.append(fmt.data(),fmt.length());
    return s;
}

// Format quantity n in SI units (k, M, G, T, P, E).
// The returned string is atmost 5 characters long.
// Requires n >= 0
string formatSI(int64_t n);

// Format quantity n in IEC (binary) units (Ki, Mi, Gi, Ti, Pi, Ei).
// The returned string is atmost 6 characters long.
// Requires n >= 0
string formatIEC(int64_t n);

}//muduo

#endif