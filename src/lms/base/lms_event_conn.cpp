#include "lms_event_conn.hpp"
#include "kernel_log.hpp"
#include "kernel_errno.hpp"
#include <sys/eventfd.h>
#include <errno.h>

lms_event_conn::lms_event_conn(DEvent *event)
    : m_fd(-1)
    , m_event(event)
{

}

lms_event_conn::~lms_event_conn()
{
    log_warn("free --> lms_event_conn");

    for (int i = 0; i < (int)m_msgs.size(); ++i) {
        DFree(m_msgs.at(i));
    }
    m_msgs.clear();    
}

bool lms_event_conn::open()
{
    m_fd = eventfd(0, EFD_NONBLOCK);
    if (m_fd == -1) {
        return false;
    }

    if (!m_event->add(this, m_fd)) {
        log_error("add eventfd to epoll failed. fd=%d", m_fd);
        return false;
    }

    return true;
}

void lms_event_conn::close()
{
    m_event->del(this, m_fd);

    if (m_fd != -1) {
        ::close(m_fd);
        m_fd = -1;
    }
}

void lms_event_conn::write(duint64 value)
{
    ::write(m_fd, &value, sizeof(duint64));
}

void lms_event_conn::addData(CommonMessage *_msg)
{
    m_lock.lock();

    CommonMessage *msg = new CommonMessage(_msg);
    m_msgs.push_back(msg);

    m_lock.unlock();

    write(1);
}

void lms_event_conn::addConnection(lms_conn_base *conn)
{
    m_sockets[conn->GetDescriptor()] = conn;
}

void lms_event_conn::delConnection(lms_conn_base *conn)
{
    std::map<int, lms_conn_base*>::iterator it;

    it = m_sockets.find(conn->GetDescriptor());
    if (it != m_sockets.end()) {
        m_sockets.erase(it);
    }
}

bool lms_event_conn::empty()
{
    return m_sockets.empty();
}

int lms_event_conn::onRead()
{
    int ret = ERROR_SUCCESS;

    duint64 value = 0;

    ::read(m_fd, &value, sizeof(duint64));

    if (value == 0) {
        return 0;
    }

    m_lock.lock();

    std::vector<CommonMessage*> msgs(m_msgs);
    m_msgs.clear();

    m_lock.unlock();

    std::map<int, lms_conn_base*>::iterator it;

    for (int i = 0; i < (int)msgs.size(); ++i)
    {
        CommonMessage *msg = msgs.at(i);

        it = m_sockets.begin();
        for(;it != m_sockets.end();) {
            lms_conn_base *conn = it->second;

            ret = conn->Process(msg);
            if ((ret != ERROR_SUCCESS) && (ret != SOCKET_EAGAIN)) {
                conn->release();
                m_sockets.erase(it++);
                continue;
            }

            it++;
        }

        DFree(msg);
    }

    return 0;
}

int lms_event_conn::onWrite()
{
    return 0;
}
