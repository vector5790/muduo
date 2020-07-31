/*
UTC 时间戳
*/
#ifndef MUDUO_BASE_TIMESTAMP_H
#define MUDUO_BASE_TIMESTAMP_H

#include "muduo/base/copyable.h"
#include "muduo/base/Types.h"

#include <boost/operators.hpp>

namespace muduo{
/*
Timestamp类继承自boost::less_than_comparable <T>模板类
只要实现 <，即可自动实现>,<=,>=
*/
class Timestamp : public muduo::copyable,
                  public boost::equality_comparable<Timestamp>,
                  public boost::less_than_comparable<Timestamp>{
public:
    Timestamp() : microSecondsSinceEpoch_(0){

    }
    /*
    Constucts a Timestamp at specific time
    */
    explicit Timestamp(int64_t microSecondsSinceEpochArg)
        : microSecondsSinceEpoch_(microSecondsSinceEpochArg){
    }

    void swap(Timestamp& that){
        std::swap(microSecondsSinceEpoch_,that.microSecondsSinceEpoch_);
    }
    string toString() const;//将时间转换为string类型
    string toFormattedString(bool showMicroseconds = true) const;//将时间转换为固定格式的string类型

    bool valid(){//判断Timestamp是否有效
        return microSecondsSinceEpoch_>0;
    }

    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
    time_t secondsSinceEpoth()const{
        return static_cast<time_t>(microSecondsSinceEpoch_/kMicroSecondPerSecond);
    }
    static Timestamp now();//返回当前时间的Timestamp
    static Timestamp invalid(){//返回一个无效的Timestamp
        return Timestamp();
    }

    static Timestamp fromUnixTime(time_t t){
        return fromUnixTime(t,0);
    }
    static Timestamp fromUnixTime(time_t t,int microseconds){
        return Timestamp(static_cast<int64_t>(t) * kMicroSecondPerSecond+microseconds);
    }
    
    static const int kMicroSecondPerSecond=1000*1000;//每秒所对应的微秒数

private:
    int64_t microSecondsSinceEpoch_;//表示到1970-01-01 00:00:00 UTC的微秒数
};

inline bool operators <(Timestamp lhs,Timestamp rhs){
    return lhs.microSecondsSinceEpoch()<rhs.microSecondsSinceEpoch();
}

inline bool operators ==(Timestamp lhs,Timestamp rhs){
    return lhs.microSecondsSinceEpoch()==rhs.microSecondsSinceEpoch();
}
inline double timeDifference(Timestamp hign,Timestamp low){
    int64_t diff=hign.microSecondsSinceEpoch()-low.microSecondsSinceEpoch();
    return static_cast<double>(diff)/Timestamp::kMicroSecondPerSecond;
}

/*
函数参数采用值传递

类对象作为参数传递并不一定采用引用传递更高效，这里采用值传递，
是因为Timestamp类只包含一个类型为int64_t的数据成员microSecondsSinceEpoch_，
所以我们可以把Timestamp对象看作是一个64位（8字节）的整数。
参数传递的过程中，会把参数传递到一个8字节的寄存器中而不是传递到堆栈当中（在对应的64位平台），它的效率会更高
*/
inline Timestamp addTime(Timestamp timestamp,double seconds){
    int64_t delta=static_cast<int64_t>(seconds * Timestamp::kMicroSecondPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch()+delta);
}

}

#endif