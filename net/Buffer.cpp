#include "Buffer.h"
#include "SocketsOps.h"
#include "../base/Logging.h"

#include <errno.h>
#include <memory.h>
#include <sys/uio.h>

using namespace muduo;
using namespace muduo::net;
/*
用户在使用Buffer接受数据实际上只会使用readFd函数，这个函数首先预备了一个64k的栈上缓冲区extrabuf，
然后使用readv函数读取数据到iovec(iovec分别指定了buffer和extrabuffer)中，
判断若buffer容量足够，则只需移动writerIndex_,否则使用append成员函数将剩余数据添加到buffer中(会执行扩容操作)
*/
ssize_t Buffer::readFd(int fd, int* savedErrno)
{
    /*
    栈额外空间,用于从套接字往出来读时，当buffer暂时不够用时暂存数据，待buffer重新分配足够空间后，在把数据交换给buffer
    */
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writable = writableBytes();
    vec[0].iov_base = begin()+writerIndex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;
    const ssize_t n = readv(fd, vec, 2);
    if (n < 0) {
        *savedErrno = errno;
    } 
    else if (implicit_cast<size_t>(n) <= writable) {
        writerIndex_ += n;
    } 
    else {
        writerIndex_ = buffer_.size();
        //将额外空间的部分加到buffer中去
        append(extrabuf, n - writable);
    }
    return n;
}