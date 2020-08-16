/*
Socket类是对文件描述符的封装，它的成员函数依然多是对SocketOps的调用
*/
#ifndef MUDUO_NET_SOCKET_H
#define MUDUO_NET_SOCKET_H

#include "../base/noncopyable.h"
namespace muduo
{
namespace net
{
class InetAddress;

class Socket : noncopyable
{
public:
    explicit Socket(int sockfd)
    : sockfd_(sockfd)
    { }

    ~Socket();
    int fd() const { return sockfd_; }
    /// abort if address in use
    void bindAddress(const InetAddress& localaddr);
    /// abort if address in use
    void listen();
    int accept(InetAddress* peeraddr);

    ///
    /// Enable/disable SO_REUSEADDR
    ///
    //是否重用端口号
    void setReuseAddr(bool on);

private:
    const int sockfd_;
};
}
}
#endif