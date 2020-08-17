#ifndef MUDUO_NET_BUFFER_H
#define MUDUO_NET_BUFFER_H

#include "../base/copyable.h"

#include <algorithm>
#include <string>
#include <vector>

#include <assert.h>
//#include <unistd.h>  // ssize_t

namespace muduo
{
namespace net{
    
class Buffer : public copyable
{
public:
    //预留区大小
    static const size_t kCheapPrepend = 8;
    //缓冲区大小
    static const size_t kInitialSize = 1024;

    Buffer()
        :buffer_(kCheapPrepend + kInitialSize),
        readerIndex_(kCheapPrepend),
        writerIndex_(kCheapPrepend)
    {
        assert(readableBytes() == 0);
        assert(writableBytes() == kInitialSize);
        assert(prependableBytes() == kCheapPrepend);
    }
    //交换俩个buffer
    void swap(Buffer& rhs)
    {
        buffer_.swap(rhs.buffer_);
        std::swap(readerIndex_, rhs.readerIndex_);
        std::swap(writerIndex_, rhs.writerIndex_);
    }
    //当前buffer中可读的字节数
    size_t readableBytes() const
    { return writerIndex_ - readerIndex_; }
    
    size_t writableBytes() const
    { return buffer_.size() - writerIndex_; }
    //返回头部预留字节数
    size_t prependableBytes() const
    { return readerIndex_; }
    //返回数据可读处
    const char* peek() const
    { return begin() + readerIndex_; }
    //从Buffer里读走len个字节的数据
    void retrieve(size_t len)
    {
        assert(len <= readableBytes());
        readerIndex_ += len;
    }
    void retrieveUntil(const char* end)
    {
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }
    void retrieveAll()
    {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }
    std::string retrieveAsString()
    {
        std::string str(peek(), readableBytes());
        retrieveAll();
        return str;
    }
    //向buffer中添加数据
    void append(const std::string& str)
    {
        append(str.data(), str.length());
    }

    void append(const char* /*restrict*/ data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data+len, beginWrite());
        hasWritten(len);
    }
    void append(const void* /*restrict*/ data, size_t len)
    {
        append(static_cast<const char*>(data), len);
    }
    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len);
        }
        assert(writableBytes() >= len);
    }
    char* beginWrite()
    { return begin() + writerIndex_; }

    const char* beginWrite() const
    { return begin() + writerIndex_; }

    void hasWritten(size_t len)
    { writerIndex_ += len; }
    //往预留区添加len数据
    void prepend(const void* /*restrict*/ data, size_t len)
    {
        assert(len <= prependableBytes());
        readerIndex_ -= len;
        const char* d = static_cast<const char*>(data);
        std::copy(d, d+len, begin()+readerIndex_);
    }
    //将写区域大小改成reserve
    void shrink(size_t reserve)
    {
        std::vector<char> buf(kCheapPrepend+readableBytes()+reserve);
        std::copy(peek(), peek()+readableBytes(), buf.begin()+kCheapPrepend);
        buf.swap(buffer_);
    }
    //从套接字读取数据到buffer
    ssize_t readFd(int fd, int* savedErrno);
private:
    char* begin()
    { return &*buffer_.begin(); }

    const char* begin() const
    { return &*buffer_.begin(); }

    void makeSpace(size_t len)
    {
        //剩余空间不足len，重新设置buffer的大小
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_+len);
        }
        else
        {
            // move readable data to the front, make space inside buffer
            /*
            因为只有写区域空间不足的时候才会调用makeSpace(),即len>writableBytes,然后进入else分支，说明
            writableBytes() + prependableBytes() >= len + kCheapPrepend
            所以kCheapPrepend < prependableBytes()  另外prependableBytes()==readerIndex_
            
            说明读区域前的预留区大小超过了kCheapPrepend，所以之后我们用copy把读数据起点左移至kCheapPrepend，
            这样剩下的读区域能容纳下len的数据   
            */
            assert(kCheapPrepend < readerIndex_);
            size_t readable = readableBytes();
            std::copy(begin()+readerIndex_,begin()+writerIndex_,begin()+kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
            assert(readable == readableBytes());
        }
    }
private:
    //vector用来保存数据
    std::vector<char> buffer_;
    /*
    将读写下标设为size_t类型，是因为如果把它们设为指针类型，那么当vector进行插入删除时都有可能让其失效
    */
    //读位置的下标
    size_t readerIndex_;
    //写位置的下标
    size_t writerIndex_;
};
}//net
}//muduo
#endif