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

DTcpSocket::DTcpSocket(DEvent *event)
    : m_event(event)
    , m_fd(-1)
    , m_connected(true)
    , m_read_buffer_length(0)
    , m_read_pos(0)
    , m_read_total_size(0)
    , m_write_pos(0)
    , m_write_buffer_len(0)
    , m_write_eagain(false)
    , m_read_timeout(-1)
    , m_write_timeout(-1)
{

}

DTcpSocket::DTcpSocket(DEvent *event, int fd)
    : m_event(event)
    , m_fd(fd)
    , m_connected(true)
    , m_read_buffer_length(0)
    , m_read_pos(0)
    , m_read_total_size(0)
    , m_write_pos(0)
    , m_write_buffer_len(0)
    , m_write_eagain(false)
    , m_read_timeout(-1)
    , m_write_timeout(-1)
{

}

DTcpSocket::~DTcpSocket()
{
    for (int i = 0; i < m_read_chunks.size(); ++i) {
        DFree(m_read_chunks.at(i));
    }
    m_read_chunks.clear();

    for (int i = 0; i < m_write_chunks.size(); ++i) {
        DFree(m_write_chunks.at(i));
    }
    m_write_chunks.clear();
}

int DTcpSocket::onRead()
{
    int ret = SOCKET_SUCCESS;

    ret = readFromFd();

    if (ret == SOCKET_CLOSE) {
        onCloseProcess();
        return ret;
    } else if (ret == SOCKET_ERROR) {
        onErrorProcess();
        return ret;
    } else {
        if ((ret = onReadProcess()) != SOCKET_SUCCESS) {
            if (ret != SOCKET_EAGAIN) {
                onErrorProcess();
            }
            return (ret == SOCKET_EAGAIN) ? SOCKET_SUCCESS : ret;
        }
    }

    return ret;
}

int DTcpSocket::onWrite()
{
    int ret = SOCKET_SUCCESS;

    if ((ret = checkConnectStatus()) != SOCKET_SUCCESS) {
        onErrorProcess();
        return ret;
    }

    m_write_eagain = false;

    // 发送时会优先发送缓冲区的数据，如果在发送缓冲区数据时遇到eagain，直接返回，不触发回调
    if (m_write_buffer_len > 0) {
        if ((ret = flush()) != SOCKET_SUCCESS) {
            if (ret != SOCKET_EAGAIN) {
                onErrorProcess();
            }
            return (ret == SOCKET_EAGAIN) ? SOCKET_SUCCESS : ret;
        }
    }

    if ((ret = onWriteProcess()) != SOCKET_SUCCESS) {
        if (ret != SOCKET_EAGAIN) {
            onErrorProcess();
        }
        return (ret == SOCKET_EAGAIN) ? SOCKET_SUCCESS : ret;
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

int DTcpSocket::GetDescriptor()
{
    return m_fd;
}

int DTcpSocket::connectToHost(const char *ip, int port)
{
    if (socket() == SOCKET_ERROR) {
        return SOCKET_ERROR;
    }

    // 如果返回SOCKET_EINPROGRESS，属于正确，需要继续加入到epoll中
    if (connect(ip, port) == SOCKET_ERROR) {
        return SOCKET_ERROR;
    }

    if (!m_event->add(this, m_fd)) {
        return SOCKET_ERROR;
    }

    return SOCKET_SUCCESS;
}

bool DTcpSocket::getConnected()
{
    return m_connected;
}

void DTcpSocket::close()
{
    m_event->delReadTimeOut(this);
    m_event->delWriteTimeOut(this);
    m_event->del(this, m_fd);

    if (m_fd != -1) {
        ::close(m_fd);
        m_fd = -1;
    }
}

int DTcpSocket::read(char *data, int length)
{
    if (length < 0) {
        return SOCKET_ERROR;
    } else if (length == 0 || m_read_buffer_length < length) {
        return SOCKET_EAGAIN;
    }

    return readDataFromBuffer(data, length);
}

int DTcpSocket::copy(char *data, int length)
{
    if (length < 0) {
        return SOCKET_ERROR;
    } else if (length == 0 || m_read_buffer_length < length) {
        return SOCKET_EAGAIN;
    }

    return copyDataFromBuffer(data, length);
}

int DTcpSocket::getReadBufferLength() const
{
    return m_read_buffer_length;
}

int DTcpSocket::checkConnectStatus()
{
    if (m_connected) {
        return SOCKET_SUCCESS;
    }

    int err;
    int errlen = sizeof(err);
    if (getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &err, (socklen_t*)&errlen) == -1) {
        return SOCKET_ERROR;
    }

    if (err != 0) {
        return SOCKET_ERROR;
    }

    m_connected = true;

    return SOCKET_SUCCESS;
}

duint64 DTcpSocket::getTotalReadSize() const
{
    return m_read_total_size;
}

int DTcpSocket::write(DSharedPtr<MemoryChunk> chunk, int length, int pos)
{
    add(chunk, length, pos);

    return flush();
}

int DTcpSocket::flush()
{
    return writeToFd();
}

void DTcpSocket::add(DSharedPtr<MemoryChunk> chunk, int length, int pos)
{
    SendBuffer *buf = new SendBuffer;

    buf->chunk = chunk;
    buf->pos = pos;
    buf->len = length;

    m_write_buffer_len += length;
    m_write_chunks.push_back(buf);
}

bool DTcpSocket::writeEagain()
{
    return m_write_eagain;
}

void DTcpSocket::setReadTimeOut(dint64 usec)
{
    m_read_timeout = usec;

    if (usec == -1) {
        m_event->delReadTimeOut(this);
    }
}

void DTcpSocket::setWriteTimeOut(dint64 usec)
{
    m_write_timeout = usec;

    if (usec == -1) {
        m_event->delWriteTimeOut(this);
    }
}

bool DTcpSocket::setTcpNodelay(bool value)
{
    int flags = (value == true) ? 1 : 0;
    if (setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags)) < 0) {
        return false;
    }

    return true;
}

bool DTcpSocket::setKeepAlive(bool value)
{
    int flags = (value == true) ? 1 : 0;
    if (setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) < 0) {
        return false;
    }

    return true;
}

bool DTcpSocket::setNonblocking()
{
    int op = fcntl(m_fd, F_GETFL, 0);
    if (fcntl(m_fd, F_SETFL, op | O_NONBLOCK) == -1) {
        return false;
    }

    return true;
}

int DTcpSocket::readFromFd()
{
    while (1) {
        MemoryChunk *chunk = DMemPool::instance()->getMemory(4096);

        int nread = ::read(m_fd, chunk->data, 4096);

        if (nread == 0) {
            DFree(chunk);
            return SOCKET_CLOSE;
        }

        if (nread < 0) {
            DFree(chunk);

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            if (errno == EINTR) {
                continue;
            }
            return SOCKET_ERROR;
        }

        chunk->length = nread;
        m_read_chunks.push_back(chunk);

        m_read_total_size += nread;

        m_read_buffer_length += nread;

        updateTimeOut(false);
    }

    return SOCKET_EAGAIN;
}

int DTcpSocket::writeToFd()
{
    if (m_write_eagain) {
        return SOCKET_EAGAIN;
    }

    while (1) {
        if (m_write_chunks.empty()){
            break;
        }

        int iovcnt = DMin(m_write_chunks.size(), 64);
        struct iovec iovs[iovcnt];
        outputBufferToIovec(iovs, iovcnt);

eintr:
        int nwrite = writev(m_fd, iovs, iovcnt);
        if (nwrite > 0) {
            m_write_buffer_len -= nwrite;
            outpufBufferUpdate(nwrite);

            updateTimeOut(true);
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                m_write_eagain = true;
                return SOCKET_EAGAIN;
            }

            if (errno == EINTR) {
                goto eintr;
            }

            return SOCKET_ERROR;
        }
    }

    return SOCKET_SUCCESS;
}

int DTcpSocket::copyDataFromBuffer(char *data, int len)
{
    int ret = 0;
    int length = len;
    int pos = 0;
    int temp_pos = m_read_pos;

    for (int i = 0; i < (int)m_read_chunks.size(); ++i) {
        if (length <= 0) {
            break;
        }

        MemoryChunk *chunk = m_read_chunks.at(i);
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

    return (ret == len) ? 0 : SOCKET_ERROR;
}

int DTcpSocket::readDataFromBuffer(char *data, int len)
{
    int ret = 0;
    int length = len;
    int pos = 0;

    while (1) {
        if (length <= 0) {
            break;
        }

        MemoryChunk *chunk = m_read_chunks.front();
        int rest = chunk->length - m_read_pos;

        if (length >= rest) {
            memcpy(data + pos, chunk->data + m_read_pos, rest);
            length -= rest;
            pos += rest;
            ret += rest;

            m_read_pos = 0;
            m_read_buffer_length -= rest;
            m_read_chunks.pop_front();
            DFree(chunk);
        } else {
            memcpy(data + pos, chunk->data + m_read_pos, length);
            ret += length;

            m_read_pos += length;
            m_read_buffer_length -= length;

            break;
        }
    }

    return (ret == len) ? 0 : SOCKET_ERROR;
}

void DTcpSocket::outputBufferToIovec(iovec *iovs, int iovcnt)
{
    struct iovec *iov;
    SendBuffer *buf;
    MemoryChunk *chunk;

    for (int i = 0; i < iovcnt; ++i) {
        iov = &iovs[i];
        buf = m_write_chunks.at(i);
        chunk = buf->chunk.get();

        if (i == 0) {
            iov->iov_base = chunk->data + buf->pos + m_write_pos;
            iov->iov_len  = buf->len - m_write_pos;
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
    int size = m_write_chunks.size();

    for (int i = 0; i < size; ++i) {
        buf = m_write_chunks.at(i);
        buf_len = buf->len - m_write_pos;

        if (i == 0 && nwrite < buf_len) {
            m_write_pos += nwrite;
            break;
        }

        if (rest >= buf_len) {
            rest = rest - buf_len;
            m_write_pos = 0;

            count++;
            DFree(buf);
        } else {
            m_write_pos = rest;
            break;
        }
    }

    if (count != 0) {
        m_write_chunks.erase(m_write_chunks.begin(), m_write_chunks.begin() + count);
    }
}

void DTcpSocket::updateTimeOut(bool send)
{
    if (send) {
        if (m_write_timeout == -1) {
            return;
        }
        m_event->addWriteTimeOut(this, m_write_timeout);
    } else {
        if (m_read_timeout == -1) {
            return;
        }
        m_event->addReadTimeOut(this, m_read_timeout);
    }
}

int DTcpSocket::socket()
{
    if ((m_fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)) == -1) {
        return SOCKET_ERROR;
    }

    return SOCKET_SUCCESS;
}

int DTcpSocket::connect(const char *ip, int port)
{
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);

    if (::connect(m_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        if (errno == EINPROGRESS) {
            m_connected = false;
            return SOCKET_EINPROGRESS;
        }
        return SOCKET_ERROR;
    }

    return SOCKET_SUCCESS;
}
