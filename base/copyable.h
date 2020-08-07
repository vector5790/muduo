/*
一个空基类，用来标识(tag)值类型
A tag class emphasises the objects are copyable.
*/
namespace muduo{

class copyable{
protected:
    //使用=default来要求编译器生成一个默认构造函数
    copyable() = default;
    ~copyable() = default;
};

}