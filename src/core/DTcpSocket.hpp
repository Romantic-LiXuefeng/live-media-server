#ifndef DTCPSOCKET_HPP
#define DTCPSOCKET_HPP

#include "DEvent.hpp"
#include "DMemPool.hpp"
#include "DSharedPtr.hpp"
#include <deque>
#include <tr1/functional>

#define TCP_SOCKET_NO_TIMEOUT   -1

struct SendBuffer
{
    // 开始位置
    int pos;
    // 长度
    int len;

    DSharedPtr<MemoryChunk> chunk;
};

/**
 * @brief 此类不需要释放，调用closeSocket函数即可
 */
class DTcpSocket : public EventHanderBase, public EventTimeOutBase
{
public:
    DTcpSocket(DEvent *event, int fd);
    DTcpSocket(DEvent *event);
    virtual ~DTcpSocket();

    // 小于0表示失败，等于0表示成功，等于EINPROGRESS表示异步connect中
    int connectToHost(const char *ip, int port);

    void closeSocket();

    bool setTcpNodelay(bool value);
    bool setKeepAlive(bool value);
    void setNonblocking();

    /**
     * @brief setSendTimeOut和setRecvTimeOut用于设置socket的发送和接收超时
     * @param msec 微妙
     */
    void setSendTimeOut(dint64 usec);
    void setRecvTimeOut(dint64 usec);

public:
    // 继承自EventHanderBase
    virtual int GetDescriptor();
    virtual int onRead();
    virtual int onWrite();

    //继承自EventTimeOutBase
    virtual void onReadTimeOut();
    virtual void onWriteTimeOut();

    // 子类实现这些函数
public:
    virtual int onReadProcess();
    virtual int onWriteProcess();
    virtual void onReadTimeOutProcess();
    virtual void onWriteTimeOutProcess();
    virtual void onErrorProcess();
    virtual void onCloseProcess();

public:
    /**
     * @brief 从socket读数据填充到自己的buffer中
     */
    int grow();
    /**
     * @brief getBufferLen 获取读缓冲区的大小
     * @return
     */
    int getBufferLen();

    /**
     * @return 如果要读的数据<=0或原始数据为空，则返回0，否则返回实际读到的大小
     */
    int readData(char *data, int len);
    /**
     * @return 如果要拷贝的数据<=0或原始数据为空，则返回0，否则返回实际读到的大小
     */
    int copyData(char *data, int len);

    duint64 get_recv_size();

    void addData(SendBuffer *data);

    int writeData();

    /**
     * @brief getWriteBufferLen 获取写缓冲区的大小
     * @return
     */
    int getWriteBufferLen();

protected:
    void outputBufferToIovec(struct iovec *iovs, int iovcnt);
    void outpufBufferUpdate(int nwrite);

    void updateTimeOut(bool send);

    /**
     * @brief ConnectStatus 获取socket的connect状态
     * @return
     */
    int ConnectStatus();

protected:
    DEvent *m_event;
    int m_fd;

    // 读缓冲区大小
    int m_buffer_len;
    //写缓冲区大小
    int m_write_buffer_len;

protected:
    std::deque<MemoryChunk*> m_recv_chunks;
    // list中第一个chunk的位置，下次再读时从此处开始
    int m_recv_pos;
    // 收到数据的总大小
    duint64 m_recv_size;

protected:
    std::deque<SendBuffer*> m_send_chunks;
    int m_send_pos;

    bool m_write_eagain;

protected:
    dint64 m_send_timeout;
    dint64 m_recv_timeout;

    bool m_connected;
};

#endif // DTCPSOCKET_HPP
