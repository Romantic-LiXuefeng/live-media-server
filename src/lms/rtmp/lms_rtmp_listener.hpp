#ifndef LMS_RTMP_LISTENER_HPP
#define LMS_RTMP_LISTENER_HPP

#include "DTcpServer.hpp"
#include "DThread.hpp"

class lms_rtmp_listener : public DTcpListener
{
public:
    lms_rtmp_listener(DEvent *event, DThread *thread);
    virtual ~lms_rtmp_listener();

protected:
    virtual EventHanderBase* onNewConnection(int fd);

private:
    DThread *m_thread;
};

#endif // LMS_RTMP_LISTENER_HPP
