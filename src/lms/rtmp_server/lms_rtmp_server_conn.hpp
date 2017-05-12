#ifndef LMS_RTMP_SERVER_CONN_HPP
#define LMS_RTMP_SERVER_CONN_HPP

#include "lms_conn_base.hpp"
#include "DThread.hpp"
#include "lms_rtmp_server_stream.hpp"
/**
 * @brief 此类不需要释放，调用release函数即可
 */
class lms_rtmp_server_conn : public lms_conn_base
{
public:
    lms_rtmp_server_conn(DThread *parent, DEvent *ev, int fd);
    virtual ~lms_rtmp_server_conn();

public:
    // Inherited from DTcpSocket
    virtual int  onReadProcess();
    virtual int  onWriteProcess();
    virtual void onReadTimeOutProcess();
    virtual void onWriteTimeOutProcess();
    virtual void onErrorProcess();
    virtual void onCloseProcess();

public:
    // Inherited from lms_conn_base
    virtual int Process(CommonMessage *_msg);
    virtual void reload();
    virtual void release();

private:
    lms_rtmp_server_stream *m_rtmp;

};

#endif // LMS_RTMP_SERVER_CONN_HPP
