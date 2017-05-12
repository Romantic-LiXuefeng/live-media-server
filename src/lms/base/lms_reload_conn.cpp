#include "lms_reload_conn.hpp"
#include <sys/eventfd.h>
#include <errno.h>
#include <algorithm>
#include "kernel_log.hpp"

lms_reload_conn::lms_reload_conn(DEvent *event)
    : m_fd(-1)
    , m_event(event)
{

}

lms_reload_conn::~lms_reload_conn()
{

}

int lms_reload_conn::open()
{
    m_fd = eventfd(0, EFD_NONBLOCK);
    m_event->add(this);

    return m_fd;
}

void lms_reload_conn::close()
{
    m_event->del(this);

    if (m_fd != -1) {
        ::close(m_fd);
        m_fd = -1;
    }
}

void lms_reload_conn::write(duint64 value)
{
    ::write(m_fd, &value, sizeof(duint64));
}

void lms_reload_conn::addConnection(lms_conn_base *conn)
{
    std::vector<lms_conn_base*>::iterator it = find(m_conns.begin(), m_conns.end(), conn);
    if (it == m_conns.end()) {
        m_conns.push_back(conn);
    }
}

void lms_reload_conn::delConnection(lms_conn_base *conn)
{
    std::vector<lms_conn_base*>::iterator it = find(m_conns.begin(), m_conns.end(), conn);
    if (it != m_conns.end()) {
        m_conns.erase(it);
    }
}

bool lms_reload_conn::empty()
{
    return m_conns.empty();
}

DEvent *lms_reload_conn::getEvent()
{
    return m_event;
}

int lms_reload_conn::GetDescriptor()
{
    return m_fd;
}

int lms_reload_conn::onRead()
{
    duint64 value = 0;

    ::read(m_fd, &value, sizeof(duint64));

    if (value == 0) {
        return 0;
    }

    for (int i = 0; i < (int)m_conns.size(); ++i) {
        lms_conn_base *conn = m_conns.at(i);
        conn->reload();
    }

    return 0;
}

int lms_reload_conn::onWrite()
{
    return 0;
}
