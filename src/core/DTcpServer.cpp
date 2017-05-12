#include "DTcpServer.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

DTcpListener::DTcpListener()
    : m_event(NULL)
    , m_fd(-1)
{

}

DTcpListener::~DTcpListener()
{

}

int DTcpListener::listen(const DString &ip, int port)
{
    m_ip = ip;
    m_port = port;

    if ((m_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)) == -1) {
        return -1;
    }

    int reuse_socket = 1;
    if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEPORT, &reuse_socket, sizeof(int)) == -1) {
        return -2;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (ip.empty()) {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
    }

    if (bind(m_fd, (sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1) {
        return -3;
    }

    if (::listen(m_fd, 128) == -1) {
        return -4;
    }

    return 0;
}

void DTcpListener::close()
{
    if (m_fd != -1) {
        m_event->del(this);

        ::close(m_fd);
        m_fd = -1;
    }
}

void DTcpListener::addToEvent(DEvent *event)
{
    m_event = event;
    m_event->add(this);
}

DEvent *DTcpListener::getEvent()
{
    return m_event;
}

int DTcpListener::GetDescriptor()
{
    return m_fd;
}

int DTcpListener::onRead()
{
    while (1) {
        sockaddr_in addr;
        socklen_t len = sizeof(addr);
        int fd = ::accept(m_fd, (sockaddr*)&addr, &len);

        if (fd > 0) {
            process(fd);
        } else {
            if (EAGAIN == errno || EWOULDBLOCK == errno) {
                break;
            }
            return -1;
        }
    }

    return 0;
}

int DTcpListener::onWrite()
{
    return 0;
}

void DTcpListener::setNonblocking(int fd)
{
    int op = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, op | O_NONBLOCK);
}

void DTcpListener::process(int fd)
{
    setNonblocking(fd);
    EventHanderBase *ev = onNewConnection(fd);
    m_event->add(ev);
}

/**************************************************************/

DTcpServer::DTcpServer()
{
    m_event = new DEvent();
}

DTcpServer::~DTcpServer()
{
    DFree(m_event);
}

bool DTcpServer::init()
{
    return m_event->init();
}

bool DTcpServer::start()
{
    return m_event->event_loop();
}

void DTcpServer::startListener(DTcpListener *listener)
{
    listener->addToEvent(m_event);
}

DEvent *DTcpServer::getEvent()
{
    return m_event;
}

