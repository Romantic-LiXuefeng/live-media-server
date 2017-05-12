#ifndef LMS_EVENT_CONN_HPP
#define LMS_EVENT_CONN_HPP

#include "DEvent.hpp"
#include "rtmp_global.hpp"
#include "lms_gop_cache.hpp"
#include "lms_conn_base.hpp"
#include "DSpinLock.hpp"
#include <map>
#include <vector>

/**
 * @brief 此类不需要释放，调用close函数即可
 */
class lms_event_conn : public EventHanderBase
{
public:
    lms_event_conn(DEvent *event);
    ~lms_event_conn();

    int open();
    void close();
    void write(duint64 value);

    void addData(CommonMessage *_msg);

    void addConnection(lms_conn_base *conn);
    void delConnection(lms_conn_base *conn);

    bool empty();

public:
    virtual int GetDescriptor();
    virtual int onRead();
    virtual int onWrite();

public:
    lms_gop_cache *gop_cache;

private:
    int m_fd;
    DEvent *m_event;

    std::map<int, lms_conn_base*> m_sockets;

    std::vector<CommonMessage*> m_msgs;
    DSpinLock m_lock;
};

#endif // LMS_EVENT_CONN_HPP
