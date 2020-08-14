/*
与定时器相关的类
*/
#ifndef MUDUO_NET_TIMERID_H
#define MUDUO_NET_TIMERID_H

#include "../base/copyable.h"

namespace muduo
{
namespace net
{

class Timer;
class TimerId : public copyable{
public:
    explicit TimerId(Timer* timer)
    : value_(timer)
    {
    }

private:
    Timer* value_;
};

}//net
}//muduo

#endif