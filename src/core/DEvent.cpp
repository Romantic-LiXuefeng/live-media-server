#include "DEvent.hpp"
#include <sys/epoll.h>
#include <string.h>
#include <algorithm>

#include "DTimer.hpp"

DEvent::DEvent()
    : m_fd(-1)
    , m_timer(NULL)
{
}

DEvent::~DEvent()
{
    close();
}

bool DEvent::init()
{
    if ((m_fd = epoll_create1(0)) == -1) {
        return false;
    }
    return true;
}

void DEvent::close()
{
    if (m_fd != -1) {
        ::close(m_fd);
        m_fd = -1;
    }
}

bool DEvent::add(EventHanderBase *handler)
{
    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.data.ptr = handler;
    event.events = EPOLLIN | EPOLLOUT | EPOLLET;

    if (epoll_ctl(m_fd, EPOLL_CTL_ADD, handler->GetDescriptor(), &event) == -1) {
        return false;
    }

    return true;
}

bool DEvent::del(EventHanderBase *handler)
{
    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.data.ptr = handler;
    event.events = EPOLLIN | EPOLLOUT | EPOLLET;

    if (epoll_ctl(m_fd, EPOLL_CTL_DEL, handler->GetDescriptor(), &event) == -1) {
        return false;
    }

    m_handlers.push_back(handler);

    return true;
}

void DEvent::remove(EventHanderBase *handler)
{
    m_handlers.push_back(handler);
}

bool DEvent::wait()
{
    EventHanderBase *handler = NULL;
    struct epoll_event events[1024];

    int count = epoll_wait(m_fd, events, 1024, -1);

    if (count > 0) {
        for (int i = 0; i < count; ++i) {
            handler = reinterpret_cast<EventHanderBase*>(events[i].data.ptr);
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

    for (int i = 0; i < (int)m_handlers.size(); ++i) {
        handler = m_handlers.at(i);
        DFree(handler);
    }
    m_handlers.clear();

    return true;
}

bool DEvent::event_loop()
{
    generateTime();

    startTimer();

    while (1) {
        wait();
    }
    return true;
}

void DEvent::addReadTimeOut(EventTimeOutBase *handler, dint64 timeout)
{
    m_read_map[handler] = m_usec + timeout;
}

void DEvent::addWriteTimeOut(EventTimeOutBase *handler, dint64 timeout)
{
    m_write_map[handler] = m_usec + timeout;
}

void DEvent::delReadTimeOut(EventTimeOutBase *handler)
{
    std::map<EventTimeOutBase*, dint64>::iterator iter = m_read_map.find(handler);

    if (iter != m_read_map.end()) {
        m_read_map.erase(iter);
    }
}

void DEvent::delWriteTimeOut(EventTimeOutBase *handler)
{
    std::map<EventTimeOutBase*, dint64>::iterator iter = m_write_map.find(handler);

    if (iter != m_write_map.end()) {
        m_write_map.erase(iter);
    }
}

void DEvent::onTimeOut()
{
    generateTime();

    EventTimeOutBase *handler = NULL;

    if (!m_read_map.empty()) {
        vector<PAIR> r_vec(m_read_map.begin(), m_read_map.end());
        sort(r_vec.begin(), r_vec.end(), CmpByValue());

        for (int i = 0; i < (int)r_vec.size(); ++i) {
            PAIR v = r_vec.at(i);

            if (m_usec >= v.second) {
                handler = v.first;
                handler->onReadTimeOut();
                m_read_map.erase(v.first);
            } else {
                break;
            }
        }
    }

    if (!m_write_map.empty()) {
        vector<PAIR> w_vec(m_write_map.begin(), m_write_map.end());
        sort(w_vec.begin(), w_vec.end(), CmpByValue());

        for (int i = 0; i < (int)w_vec.size(); ++i) {
            PAIR v = w_vec.at(i);

            if (m_usec >= v.second) {
                handler = v.first;
                handler->onWriteTimeOut();
                m_write_map.erase(v.first);
            } else {
                break;
            }
        }
    }
}

void DEvent::generateTime()
{
    m_monotonic = DDateTime::monotonic();
    m_usec = m_monotonic.tv_sec * 1000 * 1000 + m_monotonic.tv_nsec / 1000;
}

void DEvent::startTimer()
{
    m_timer = new DTimer(this);
    m_timer->setTimerEvent(TIMER_CALLBACK(&DEvent::onTimeOut));
    m_timer->start(50);
}
