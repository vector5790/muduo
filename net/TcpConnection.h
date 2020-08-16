#ifndef MUDUO_NET_TCPCONNECTION_H
#define MUDUO_NET_TCPCONNECTION_H

#include "Callbacks.h"
#include "InetAddress.h"
#include "../base/noncopyable.h"

#include <boost/any.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

namespace muduo
{
namespace net
{
class Channel;
class EventLoop;
class Socket;
class TcpConnection : noncopyable,
                      public boost::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop,
                const std::string& name,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr);
    ~TcpConnection();
    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() { return localAddr_; }
    const InetAddress& peerAddress() { return peerAddr_; }
    bool connected() const { return state_ == kConnected; }
    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }
    void connectEstablished();
private:
    //目前只有两个状态
    enum StateE { kConnecting, kConnected, };

    void setState(StateE s) { state_ = s; }
    void handleRead();

    EventLoop* loop_;
    std::string name_;
    StateE state_;  // FIXME: use atomic variable
    // we don't expose those classes to client.
    /*
    socket_是一个Socket类指针，指向的Socket的socket的文件描述符便是与客户端通信的connfd
    */
    boost::scoped_ptr<Socket> socket_;
    /*
    channel_的作用是，当建立连接时，将connfd与channel_绑定，然后将channel_加入到poller中，方便后续的通信
    */
    boost::scoped_ptr<Channel> channel_;
    InetAddress localAddr_;
    InetAddress peerAddr_;
    /*
    以下两个的成员类型的全称在Callbacks.h
    实际上会由TcpServer设置，分别在建立连接和通信时调用
    */
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
};
}//net
}//muduo
#endif