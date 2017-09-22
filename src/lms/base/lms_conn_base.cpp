#include "lms_conn_base.hpp"

lms_conn_base::lms_conn_base(DThread *thread, DEvent *event, int fd)
    : DTcpSocket(event, fd)
    , m_thread(thread)
{

}

lms_conn_base::~lms_conn_base()
{

}

DEvent *lms_conn_base::getEvent()
{
    return m_event;
}

pthread_t lms_conn_base::getThread()
{
    return m_thread->thread_id();
}
