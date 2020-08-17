#include "TcpConnection.h"

#include "../base/Logging.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "SocketsOps.h"

#include <boost/bind.hpp>

#include <errno.h>
#include <stdio.h>
#include <string.h>

using namespace muduo;
using namespace muduo::net;
TcpConnection::TcpConnection(EventLoop* loop,
                             const std::string& nameArg,
                             int sockfd,
                             const InetAddress& localAddr,
                             const InetAddress& peerAddr)
  : loop_(loop),
    name_(nameArg),
    state_(kConnecting),
    socket_(new Socket(sockfd)),
    channel_(new Channel(loop, sockfd)),
    localAddr_(localAddr),
    peerAddr_(peerAddr)
{
    LOG_DEBUG << "TcpConnection::ctor[" <<  name_ << "] at " << this
            << " fd=" << sockfd;
    channel_->setReadCallback(boost::bind(&TcpConnection::handleRead, this,_1));
    channel_->setWriteCallback(
        boost::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
        boost::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
        boost::bind(&TcpConnection::handleError, this));
}
TcpConnection::~TcpConnection()
{
    LOG_DEBUG << "TcpConnection::dtor[" <<  name_ << "] at " << this
            << " fd=" << channel_->fd();
}
void TcpConnection::send(const std::string& message){
    if(state_==kConnected){
        if(loop_->isInLoopThread()){
            sendInLoop(message);
        }
        else{
            loop_->runInLoop(boost::bind(&TcpConnection::sendInLoop,this,message));
        }
    }
}
void TcpConnection::sendInLoop(const std::string& message){
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    // if no thing in output queue, try writing directly
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = ::write(channel_->fd(), message.data(), message.size());
        if (nwrote >= 0) {
            if (implicit_cast<size_t>(nwrote) < message.size()) {
                LOG_TRACE << "I am going to write more data";
            }
        } else {
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                LOG_SYSERR << "TcpConnection::sendInLoop";
            }
        }
    }
    assert(nwrote >= 0);
    //输出剩下的内容
    if (implicit_cast<size_t>(nwrote) < message.size()) {
        outputBuffer_.append(message.data()+nwrote, message.size()-nwrote);
        if (!channel_->isWriting()) {
            channel_->enableWriting();
        }
    }
}
void TcpConnection::shutdown(){
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        // FIXME: shared_from_this()?
        loop_->runInLoop(boost::bind(&TcpConnection::shutdownInLoop, this));
    }
}
void TcpConnection::shutdownInLoop(){
    loop_->assertInLoopThread();
    if (!channel_->isWriting())
    {
        // we are not writing
        socket_->shutdownWrite();
    }
}
void TcpConnection::connectEstablished()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}
/*
是TcpConnection析构前最后调用的一个成员函数，他通知用户连接已断开。
在某些情况下可以不经由handleClose()而直接调用connectDestroyed
*/
void TcpConnection::connectDestroyed()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnected|| state_ == kDisconnecting);
    setState(kDisconnected);
    channel_->disableAll();
    connectionCallback_(shared_from_this());

    loop_->removeChannel(get_pointer(channel_));
}
//会检查read()的返回值，根据返回值分别调用messageCallback_,handleClose,handleError()
void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno=0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(),&savedErrno);
    if (n > 0) {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
        // FIXME: close connection if n == 0
    }
    else if(n==0) {
        handleClose();
    }
    else{
        errno=savedErrno;
        LOG_SYSERR<<"TcpConnection::handleRead";
        handleError();
    } 
}

void TcpConnection::handleWrite()
{
    loop_->assertInLoopThread();
    if (channel_->isWriting()) {
        ssize_t n = ::write(channel_->fd(),outputBuffer_.peek(),outputBuffer_.readableBytes());
        if (n > 0) {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) {
                channel_->disableWriting();
                if (state_ == kDisconnecting) {
                    shutdownInLoop();
                }
            } 
            else {
                LOG_TRACE << "I am going to write more data";
            }
        } 
        else {
            LOG_SYSERR << "TcpConnection::handleWrite";
        }
    } 
    else {
        LOG_TRACE << "Connection is down, no more writing";
    }
}
//主要是调用closeCallback_,这个回调绑定到TcpServer::removeConnection()
void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    LOG_TRACE << "TcpConnection::handleClose state = " << state_;
    assert(state_ == kConnected|| state_ == kDisconnecting);
    // we don't close fd, leave it to dtor, so we can find leaks easily.
    channel_->disableAll();
    // must be the last line
    closeCallback_(shared_from_this());
}

void TcpConnection::handleError()
{
    int err = sockets::getSocketError(channel_->fd());
    LOG_ERROR << "TcpConnection::handleError [" << name_
            << "] - SO_ERROR = " << err << " " ;
    //<< strerror_tl(err);
    //原代码中最后有strerror_tl(err) 一直编译不过，不知道是哪个头文件，待修复
}