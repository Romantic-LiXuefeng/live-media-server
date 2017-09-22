#include "lms_rtmp_listener.hpp"
#include "kernel_log.hpp"
#include "lms_rtmp_server_conn.hpp"
#include "kernel_utility.hpp"

lms_rtmp_listener::lms_rtmp_listener(DEvent *event, DThread *thread)
    : DTcpListener(event)
    , m_thread(thread)
{

}

lms_rtmp_listener::~lms_rtmp_listener()
{

}

EventHanderBase *lms_rtmp_listener::onNewConnection(int fd)
{
    global_context->set_id(fd);
    global_context->update_id(fd);

    log_trace("rtmp new connection arrived, fd=%d, client_ip=%s", fd, get_peer_ip(fd).c_str());

    lms_rtmp_server_conn *conn = new lms_rtmp_server_conn(m_thread, m_event, fd);
    return dynamic_cast<EventHanderBase*>(conn);
}

