#ifndef LMS_HTTP_LISTENER_HPP
#define LMS_HTTP_LISTENER_HPP

#include "DTcpServer.hpp"
#include "DThread.hpp"

class lms_http_listener : public DTcpListener
{
public:
    lms_http_listener(DThread *thread);
    virtual ~lms_http_listener();

protected:
    virtual EventHanderBase* onNewConnection(int fd);

private:
    DThread *m_thread;

};

#endif // LMS_HTTP_LISTENER_HPP
