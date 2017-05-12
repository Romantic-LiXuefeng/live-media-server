#include "lms_conn_base.hpp"

lms_conn_base::lms_conn_base(DThread *thread, DEvent *event, int fd)
    : DTcpSocket(event, fd)
    , m_thread(thread)
    , m_gop_cache(NULL)
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

void lms_conn_base::setGopCache(lms_gop_cache *gop)
{
    m_gop_cache = gop;
}

lms_gop_cache *lms_conn_base::getGopCache()
{
    return m_gop_cache;
}
