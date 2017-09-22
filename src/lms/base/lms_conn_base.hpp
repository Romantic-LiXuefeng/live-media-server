#ifndef LMS_CONN_BASE_HPP
#define LMS_CONN_BASE_HPP

#include "DThread.hpp"
#include "DEvent.hpp"
#include "DTcpSocket.hpp"

#include "kernel_global.hpp"

class lms_conn_base : public DTcpSocket
{
public:
    lms_conn_base(DThread *thread, DEvent *event, int fd);
    virtual ~lms_conn_base();

public:
    virtual int Process(CommonMessage *msg) = 0;
    virtual void reload() = 0;
    virtual void release() = 0;

    DEvent *getEvent();
    pthread_t getThread();

protected:
    DThread *m_thread;

};

#endif // LMS_CONN_BASE_HPP
