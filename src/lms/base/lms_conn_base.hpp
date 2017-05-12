#ifndef LMS_CONN_BASE_HPP
#define LMS_CONN_BASE_HPP

#include "DThread.hpp"
#include "DEvent.hpp"
#include "DTcpSocket.hpp"

#include "rtmp_global.hpp"
#include "lms_gop_cache.hpp"

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

    void setGopCache(lms_gop_cache *gop);
    lms_gop_cache *getGopCache();

protected:
    DThread *m_thread;
    lms_gop_cache *m_gop_cache;

};

#endif // LMS_CONN_BASE_HPP
