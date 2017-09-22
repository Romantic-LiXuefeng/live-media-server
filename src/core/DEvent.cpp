#include "DEvent.hpp"
#include <sys/epoll.h>
#include <string.h>
#include <algorithm>

#include "DTimer.hpp"

DEvent::DEvent()
    : m_timer(NULL)
{
    m_fd = epoll_create1(0);
    if (m_fd == -1) {
        fprintf(stderr, "epoll_create1 return fd is invalid");
        ::exit(-1);
    }
}

DEvent::~DEvent()
{
    close();
}

void DEvent::start()
{
    generateMonotonicTime();

    startTimer();

    while (1) {
        wait();
        freeDelHandlers();
    }
}

void DEvent::close()
{
    if (m_fd != -1) {
        ::close(m_fd);
        m_fd = -1;
    }
}

int DEvent::GetDescriptor()
{
    return m_fd;
}

bool DEvent::add(EventHanderBase *handler, int fd)
{
    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.data.ptr = handler;
    event.events = EPOLLIN | EPOLLOUT | EPOLLET;

    if (epoll_ctl(m_fd, EPOLL_CTL_ADD, fd, &event) == -1) {
        return false;
    }

    return true;
}

bool DEvent::del(EventHanderBase *handler, int fd)
{
    std::vector<EventHanderBase*>::iterator it = find(m_del_handlers.begin(), m_del_handlers.end(), handler);
    if (it == m_del_handlers.end()) {
        m_del_handlers.push_back(handler);
    }

    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.data.ptr = handler;
    event.events = EPOLLIN | EPOLLOUT | EPOLLET;

    if (epoll_ctl(m_fd, EPOLL_CTL_DEL, fd, &event) == -1) {
        return false;
    }

    return true;
}

void DEvent::addReadTimeOut(EventTimeOutBase *handler, dint64 timeout)
{
    m_read_timeout_handlers[handler] = m_usec + timeout;
}

void DEvent::addWriteTimeOut(EventTimeOutBase *handler, dint64 timeout)
{
    m_write_timeout_handlers[handler] = m_usec + timeout;
}

void DEvent::delReadTimeOut(EventTimeOutBase *handler)
{
    std::map<EventTimeOutBase*, dint64>::iterator iter = m_read_timeout_handlers.find(handler);

    if (iter != m_read_timeout_handlers.end()) {
        m_read_timeout_handlers.erase(iter);
    }
}

void DEvent::delWriteTimeOut(EventTimeOutBase *handler)
{
    std::map<EventTimeOutBase*, dint64>::iterator iter = m_write_timeout_handlers.find(handler);

    if (iter != m_write_timeout_handlers.end()) {
        m_write_timeout_handlers.erase(iter);
    }
}

void DEvent::onTimeOut()
{
    generateMonotonicTime();

    if (!m_read_timeout_handlers.empty()) {
        vector<PAIR> r_vec(m_read_timeout_handlers.begin(), m_read_timeout_handlers.end());
        sort(r_vec.begin(), r_vec.end(), CmpByValue());

        for (int i = 0; i < (int)r_vec.size(); ++i) {
            PAIR v = r_vec.at(i);

            if (m_usec >= v.second) {
                EventTimeOutBase * handler = v.first;
                handler->onReadTimeOut();
                m_read_timeout_handlers.erase(v.first);
            } else {
                break;
            }
        }
    }

    if (!m_write_timeout_handlers.empty()) {
        vector<PAIR> w_vec(m_write_timeout_handlers.begin(), m_write_timeout_handlers.end());
        sort(w_vec.begin(), w_vec.end(), CmpByValue());

        for (int i = 0; i < (int)w_vec.size(); ++i) {
            PAIR v = w_vec.at(i);

            if (m_usec >= v.second) {
                EventTimeOutBase *handler = v.first;
                handler->onWriteTimeOut();
                m_write_timeout_handlers.erase(v.first);
            } else {
                break;
            }
        }
    }
}

void DEvent::generateMonotonicTime()
{
    m_monotonic = DDateTime::monotonic();
    m_usec = m_monotonic.tv_sec * 1000 * 1000 + m_monotonic.tv_nsec / 1000;
}

void DEvent::startTimer()
{
    m_timer = new DTimer(this);
    m_timer->setTimerEvent(TIMER_CALLBACK(&DEvent::onTimeOut));

    bool ret = m_timer->start(20);
    if (!ret) {
        fprintf(stderr, "DEvent start timer failed\n");
        ::exit(-1);
    }
}

void DEvent::wait()
{
    struct epoll_event events[1024];

    int count = epoll_wait(m_fd, events, 1024, -1);

    if (count > 0) {
        for (int i = 0; i < count; ++i) {
            EventHanderBase *handler = reinterpret_cast<EventHanderBase*>(events[i].data.ptr);
            if (!handler) {
                continue;
            }

            duint32 event = events[i].events;
            if (event & EPOLLIN || event & EPOLLERR || event & EPOLLHUP) {
                if (handler->onRead() != 0) {
                    continue;
                }
            }
            if (event & EPOLLOUT) {
                if (handler->onWrite() != 0) {
                    continue;
                }
            }
        }
    }
}

void DEvent::freeDelHandlers()
{
    for (int i = 0; i < (int)m_del_handlers.size(); ++i) {
        EventHanderBase *handler = m_del_handlers.at(i);
        DFree(handler);
    }
    m_del_handlers.clear();
}
