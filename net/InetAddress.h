/*
对struct sockaddr_in的封装
*/
#ifndef MUDUO_NET_INETADDRESS_H
#define MUDUO_NET_INETADDRESS_H

#include "../base/copyable.h"

#include <string>

#include <netinet/in.h>

namespace muduo
{
namespace net
{
class InetAddress : public copyable
{
public:
/*
对于构造函数
可以只传入端口号，那么addr_的ip将会是本机(INADDR_ANY)。也可以传入ip和端口号来进行设置
*/
    /// Constructs an endpoint with given port number.
    /// Mostly used in TcpServer listening.
    explicit InetAddress(uint16_t port);

    /// Constructs an endpoint with given ip and port.
    /// @c ip should be "1.2.3.4"
    InetAddress(const std::string& ip, uint16_t port);
    /// Constructs an endpoint with given struct @c sockaddr_in
    /// Mostly used when accepting new connections
    InetAddress(const struct sockaddr_in& addr)
        : addr_(addr)
    { }
    std::string toHostPort() const;
    // default copy/assignment are Okay
    const struct sockaddr_in& getSockAddrInet() const { return addr_; }
    void setSockAddrInet(const struct sockaddr_in& addr) { addr_ = addr; }
private:
  struct sockaddr_in addr_;
};
}//net
}//muduo
#endif