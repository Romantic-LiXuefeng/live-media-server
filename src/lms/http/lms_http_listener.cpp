#include "lms_http_listener.hpp"
#include "kernel_log.hpp"
#include "lms_http_server_conn.hpp"
#include "kernel_utility.hpp"

lms_http_listener::lms_http_listener(DEvent *event, DThread *thread)
    : DTcpListener(event)
    , m_thread(thread)
{

}

lms_http_listener::~lms_http_listener()
{

}

EventHanderBase *lms_http_listener::onNewConnection(int fd)
{
    global_context->set_id(fd);
    global_context->update_id(fd);

    log_trace("http new connection arrived, fd=%d, client_ip=%s", fd, get_peer_ip(fd).c_str());

    lms_http_server_conn *conn = new lms_http_server_conn(m_thread, m_event, fd);
    return dynamic_cast<EventHanderBase*>(conn);
}
