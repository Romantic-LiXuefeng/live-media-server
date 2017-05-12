#include "DTcpSocket.hpp"
#include "DGlobal.hpp"

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#define LMS_ERROR   -1
#define LMS_CLOSE   -2

DTcpSocket::DTcpSocket(DEvent *event, int fd)
    : m_event(event)
    , m_fd(fd)
    , m_buffer_len(0)
    , m_write_buffer_len(0)
    , m_recv_pos(0)
    , m_recv_size(0)
    , m_send_pos(0)
    , m_write_eagain(false)
    , m_send_timeout(-1)
    , m_recv_timeout(-1)
    , m_connected(true)
{

}

DTcpSocket::DTcpSocket(DEvent *event)
    : m_event(event)
    , m_fd(-1)
    , m_buffer_len(0)
    , m_write_buffer_len(0)
    , m_recv_pos(0)
    , m_recv_size(0)
    , m_send_pos(0)
    , m_write_eagain(false)
    , m_send_timeout(-1)
    , m_recv_timeout(-1)
    , m_connected(true)
{

}

DTcpSocket::~DTcpSocket()
{
    for (int i = 0; i < (int)m_recv_chunks.size(); ++i) {
        DFree(m_recv_chunks.at(i));
    }
    m_recv_chunks.clear();

    for (int i = 0; i < (int)m_send_chunks.size(); ++i) {
        DFree(m_send_chunks.at(i));
    }
    m_send_chunks.clear();
}

int DTcpSocket::connectToHost(const char *ip, int port)
{
    if ((m_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)) == -1) {
        m_event->remove(this);
        return -1;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);

    m_event->add(this);

    if (connect(m_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        if (errno != EINPROGRESS) {
            return -2;
        } else {
            m_connected = false;
            return EINPROGRESS;
        }
    }

    return 0;
}

void DTcpSocket::closeSocket()
{
    if (m_fd != -1) {
        m_event->delReadTimeOut(this);
        m_event->delWriteTimeOut(this);

        m_event->del(this);

        ::close(m_fd);
        m_fd = -1;
    }
}

bool DTcpSocket::setTcpNodelay(bool value)
{
    int flags = (value == true) ? 1 : 0;
    if (setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags)) != 0) {
        return false;
    }
    return true;
}

bool DTcpSocket::setKeepAlive(bool value)
{
    int flags = (value == true) ? 1 : 0;
    if (setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) != 0) {
        return false;
    }
    return true;
}

void DTcpSocket::setNonblocking()
{
    int op = fcntl(m_fd, F_GETFL, 0);
    fcntl(m_fd, F_SETFL, op | O_NONBLOCK);
}

void DTcpSocket::setSendTimeOut(dint64 usec)
{
    m_send_timeout = usec;

    if (usec == -1) {
        m_event->delWriteTimeOut(this);
    }
}

void DTcpSocket::setRecvTimeOut(dint64 usec)
{
    m_recv_timeout = usec;

    if (usec == -1) {
        m_event->delReadTimeOut(this);
    }
}

int DTcpSocket::GetDescriptor()
{
    return m_fd;
}

int DTcpSocket::onRead()
{
    int ret = 0;

    ret = grow();
    if (ret == LMS_CLOSE) {
        onCloseProcess();
        return ret;
    }
    if (ret == LMS_ERROR) {
        onErrorProcess();
        return ret;
    }

    if ((ret = onReadProcess()) != 0) {
        onErrorProcess();
        return ret;
    }

    return ret;
}

int DTcpSocket::onWrite()
{
    int ret = 0;

    if ((ret = ConnectStatus()) != 0) {
        onErrorProcess();
        return ret;
    }

    m_write_eagain = false;

    if ((ret = onWriteProcess()) != 0) {
        onErrorProcess();
        return ret;
    }

    return ret;
}

void DTcpSocket::onReadTimeOut()
{
    onReadTimeOutProcess();
}

void DTcpSocket::onWriteTimeOut()
{
    onWriteTimeOutProcess();
}

int DTcpSocket::onReadProcess()
{
    return 0;
}

int DTcpSocket::onWriteProcess()
{
    return 0;
}

void DTcpSocket::onReadTimeOutProcess()
{
    closeSocket();
}

void DTcpSocket::onWriteTimeOutProcess()
{
    closeSocket();
}

void DTcpSocket::onErrorProcess()
{
    closeSocket();
}

void DTcpSocket::onCloseProcess()
{
    closeSocket();
}

int DTcpSocket::grow()
{
    int ret = 0;

    while (1) {
        MemoryChunk *chunk = DMemPool::instance()->getMemory(4096);

        int nread = read(m_fd, chunk->data, 4096);

        if (nread == 0) {
            DFree(chunk);
            return LMS_CLOSE;
        }

        if (nread < 0) {
            DFree(chunk);

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            if (errno == EINTR) {
                continue;
            }
            return LMS_ERROR;
        }

        chunk->length = nread;
        m_recv_chunks.push_back(chunk);

        m_recv_size += nread;

        m_buffer_len += nread;
    }

    updateTimeOut(false);

    return ret;
}

int DTcpSocket::getBufferLen()
{
    return m_buffer_len;
}

int DTcpSocket::readData(char *data, int len)
{
    int ret = 0;
    int length = len;
    int pos = 0;

    while (1) {
        if (m_recv_chunks.empty() && ret == 0) {
            ret = -1;
            break;
        }

        if (length <= 0) {
            break;
        }

        MemoryChunk *chunk = m_recv_chunks.front();
        int rest = chunk->length - m_recv_pos;

        if (length >= rest) {
            memcpy(data + pos, chunk->data + m_recv_pos, rest);
            length -= rest;
            pos += rest;
            m_recv_pos = 0;
            ret += rest;

            m_buffer_len -= rest;

            m_recv_chunks.pop_front();
            DFree(chunk);
        } else {
            memcpy(data + pos, chunk->data + m_recv_pos, length);
            m_recv_pos += length;
            ret += length;

            m_buffer_len -= length;

            break;
        }
    }

    return ret;
}

int DTcpSocket::copyData(char *data, int len)
{
    int ret = 0;
    int length = len;
    int pos = 0;
    int temp_pos = m_recv_pos;

    if (m_buffer_len < len) {
        return -1;
    }

    for (int i = 0; i < (int)m_recv_chunks.size(); ++i) {
        if (length <= 0) {
            break;
        }

        MemoryChunk *chunk = m_recv_chunks.at(i);
        int rest = chunk->length - temp_pos;

        if (length >= rest) {
            memcpy(data + pos, chunk->data + temp_pos, rest);
            length -= rest;
            pos += rest;
            temp_pos = 0;
            ret += rest;
        } else {
            memcpy(data + pos, chunk->data + temp_pos, length);
            temp_pos += length;
            ret += length;
            break;
        }
    }

    return ret;
}

duint64 DTcpSocket::get_recv_size()
{
    return m_recv_size;
}

void DTcpSocket::addData(SendBuffer *data)
{
    m_write_buffer_len += data->len;
    m_send_chunks.push_back(data);
}

int DTcpSocket::writeData()
{
    int ret = 0;

    if (m_write_eagain || m_fd == -1) {
        return ret;
    }

    while (1) {
        if (m_send_chunks.empty()){
            break;
        }

        int iovcnt = DMin(m_send_chunks.size(), 64);
        struct iovec iovs[iovcnt];
        outputBufferToIovec(iovs, iovcnt);

eintr:
        int nwrite = writev(m_fd, iovs, iovcnt);
        if (nwrite > 0) {
            m_write_buffer_len -= nwrite;
            outpufBufferUpdate(nwrite);
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                m_write_eagain = true;
                break;
            }

            if (errno == EINTR) {
                goto eintr;
            }

            return LMS_ERROR;
        }
    }

    updateTimeOut(true);

    return ret;
}

int DTcpSocket::getWriteBufferLen()
{
    return m_write_buffer_len;
}

void DTcpSocket::outputBufferToIovec(struct iovec *iovs, int iovcnt)
{
    struct iovec *iov;
    SendBuffer *buf;
    MemoryChunk *chunk;

    for (int i = 0; i < iovcnt; ++i) {
        iov = &iovs[i];
        buf = m_send_chunks.at(i);
        chunk = buf->chunk.get();

        if (i == 0) {
            iov->iov_base = chunk->data + buf->pos + m_send_pos;
            iov->iov_len  = buf->len - m_send_pos;
        } else {
            iov->iov_base = chunk->data + buf->pos;
            iov->iov_len  = buf->len;
        }
    }
}

void DTcpSocket::outpufBufferUpdate(int nwrite)
{
    SendBuffer *buf;
    int buf_len;
    int rest = nwrite;
    int count = 0;
    int size = m_send_chunks.size();

    for (int i = 0; i < size; ++i) {
        buf = m_send_chunks.at(i);
        buf_len = buf->len - m_send_pos;

        if (i == 0 && nwrite < buf_len) {
            m_send_pos += nwrite;
            break;
        }

        if (rest >= buf_len) {
            rest = rest - buf_len;
            m_send_pos = 0;

            count++;
            DFree(buf);
        } else {
            m_send_pos = rest;
            break;
        }
    }

    if (count != 0) {
        m_send_chunks.erase(m_send_chunks.begin(), m_send_chunks.begin() + count);
    }
}

void DTcpSocket::updateTimeOut(bool send)
{
    if (send) {
        if (m_send_timeout == -1) {
            return;
        }
        m_event->addWriteTimeOut(this, m_send_timeout);
    } else {
        if (m_recv_timeout == -1) {
            return;
        }
        m_event->addReadTimeOut(this, m_recv_timeout);
    }
}

int DTcpSocket::ConnectStatus()
{
    if (m_connected) {
        return 0;
    }

    int err;
    int errlen = sizeof(err);
    if (getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &err, (socklen_t*)&errlen) == -1) {
        return -1;
    }

    if (err != 0) {
        return err;
    }

    m_connected = true;

    return 0;
}

