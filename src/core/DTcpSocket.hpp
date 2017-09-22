#ifndef DTCPSOCKET_HPP
#define DTCPSOCKET_HPP

#include "DEvent.hpp"
#include "DMemPool.hpp"
#include "DSharedPtr.hpp"
#include <deque>
#include <tr1/functional>

#define SOCKET_SUCCESS       0
#define SOCKET_ERROR        -1
#define SOCKET_CLOSE        -2
#define SOCKET_EINPROGRESS  -3
#define SOCKET_EAGAIN       -100

class DTcpSocket;

struct SendBuffer
{
    // 开始位置
    int pos;
    // 长度
    int len;

    DSharedPtr<MemoryChunk> chunk;
};

class DTcpSocket : public EventHanderBase, public EventTimeOutBase
{
public:
    DTcpSocket(DEvent *event);
    DTcpSocket(DEvent *event, int fd);
    virtual ~DTcpSocket();

public:
    // Inherited from EventHanderBase
    virtual int onRead();
    virtual int onWrite();

    // Inherited from EventTimeOutBase
    virtual void onReadTimeOut();
    virtual void onWriteTimeOut();

protected:
    // When successful, returns zero.
    virtual int onReadProcess() = 0;
    virtual int onWriteProcess() = 0;
    virtual void onErrorProcess() = 0;
    virtual void onCloseProcess() = 0;
    virtual void onReadTimeOutProcess() = 0;
    virtual void onWriteTimeOutProcess() = 0;

public:
    /**
     * @brief GetDescriptor 获取文件描述符
     * @return
     */
    int GetDescriptor();
    /**
     * @brief connectToHost
     * @param ip
     * @param port
     * @return 成功返回0，失败返回-1
     */
    int connectToHost(const char *ip, int port);

    bool getConnected();

    /**
     * @brief close 调用close会自动析构socket对象，不再需要额外释放
     */
    void close();

    /**
     * @brief read
     * @param data
     * @param len
     * @return 成功返回0，失败返回-1，要读的数据为0或者buffer中的数据少于length，则不读，返回SOCKET_EAGAIN
     */
    int read(char *data, int length);
    /**
     * @brief copy 从buffer中拷贝数据，不真正读取
     * @param data
     * @param len
     * @return  成功返回0，失败返回-1，要读的数据为0或者buffer中的数据少于length，则不读，返回SOCKET_EAGAIN
     */
    int copy(char *data, int length);

    /**
     * @brief getReadBufferLength 获取读缓冲区中的数据长度
     * @return
     */
    int getReadBufferLength() const;

    /**
     * @brief getTotalReadSize 获取socket收到的全部数据的大小
     * @return
     */
    duint64 getTotalReadSize() const;

    /**
     * @brief write
     * @param chunk
     * @param pos
     * @param length
     * @return 成功返回0，失败返回-1，遇到EAGAIN返回SOCKET_EAGAIN(-100)
     */
    int write(DSharedPtr<MemoryChunk> chunk, int length, int pos = 0);
    /**
     * @brief write 发送队列中的数据
     * @return 成功返回0，失败返回-1，遇到EAGAIN返回SOCKET_EAGAIN(-100)
     */
    int flush();

    void add(DSharedPtr<MemoryChunk> chunk, int length, int pos = 0);

    bool writeEagain();

    void setReadTimeOut(dint64 usec);
    void setWriteTimeOut(dint64 usec);

    bool setTcpNodelay(bool value);
    bool setKeepAlive(bool value);
    bool setNonblocking();

protected:
    /**
     * @brief readFromFd 从系统socket的fd中读数据
     * @return 成功返回SOCKET_EAGAIN，失败返回SOCKET_ERROR或SOCKET_CLOSE
     */
    int readFromFd();
    /**
     * @brief writeToFd 向系统socket的fd中写数据
     * @return 成功返回0，失败返回-1，遇到EAGAIN返回SOCKET_EAGAIN(-100)
     */
    int writeToFd();

protected:
    int copyDataFromBuffer(char *data, int len);
    int readDataFromBuffer(char *data, int len);

    void outputBufferToIovec(struct iovec *iovs, int iovcnt);
    void outpufBufferUpdate(int nwrite);

    void updateTimeOut(bool send);

    /**
     * @brief socket
     * @return 成功返回0，失败返回-1
     */
    int socket();
    /**
     * @brief connect
     * @param ip
     * @param port
     * @return 成功返回0，失败返回-1，errno为EINPROGRESS时，返回SOCKET_EINPROGRESS
     */
    int connect(const char *ip, int port);

    int checkConnectStatus();

protected:
    DEvent *m_event;
    int m_fd;
    bool m_connected;

protected:
    std::deque<MemoryChunk*> m_read_chunks;

    int m_read_buffer_length;
    // list中第一个chunk的位置，下次再读时从此处开始
    int m_read_pos;
    // 收到数据的总大小
    duint64 m_read_total_size;

protected:
    std::deque<SendBuffer*> m_write_chunks;
    int m_write_pos;
    int m_write_buffer_len;

    bool m_write_eagain;

protected:
    dint64 m_read_timeout;
    dint64 m_write_timeout;

};

#endif // DTCPSOCKET_HPP
