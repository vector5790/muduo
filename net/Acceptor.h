/*
Acceptor用于接受新的TCP连接，通过回调通知使用者。它由TcpServer(后面讲)使用。
它的封装原理类似于TimerQueue，我们使用它的时候，
首先将它注册到EvetLoop中，Acceptor的acceptChannel_将加入Eventloop的Poller中，
至于我们就可以监听来自其他Socket的连接了
*/
#ifndef MUDUO_NET_ACCEPTOR_H
#define MUDUO_NET_ACCEPTOR_H

#include <boost/function.hpp>
#include "../base/noncopyable.h"

#include "Channel.h"
#include "Socket.h"

namespace muduo
{
namespace net{

class EventLoop;
class InetAddress;

///
/// Acceptor of incoming TCP connections.
///
class Acceptor : noncopyable
{
public:
    typedef boost::function<void (int sockfd,const InetAddress&)> NewConnectionCallback;
    Acceptor(EventLoop* loop, const InetAddress& listenAddr);
    void setNewConnectionCallback(const NewConnectionCallback& cb)
    { newConnectionCallback_ = cb; }
    bool listenning() const { return listenning_; }
    void listen();

private:
    void handleRead();

    EventLoop* loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
};
}//net
}//muduo
#endif