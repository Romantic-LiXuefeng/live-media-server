#ifndef LMS_RELOAD_CONN_HPP
#define LMS_RELOAD_CONN_HPP

#include "DEvent.hpp"
#include <vector>
#include "lms_conn_base.hpp"

class lms_reload_conn : public EventHanderBase
{
public:
    lms_reload_conn(DEvent *event);
    ~lms_reload_conn();

    bool open();
    void close();
    void write(duint64 value);

    void addConnection(lms_conn_base *conn);
    void delConnection(lms_conn_base *conn);

    bool empty();

    DEvent *getEvent();

public:
    virtual int onRead();
    virtual int onWrite();

private:
    int m_fd;
    DEvent *m_event;

    std::vector<lms_conn_base*> m_conns;
};

#endif // LMS_RELOAD_CONN_HPP
