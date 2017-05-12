#ifndef LMS_HTTP_SERVER_CONN_HPP
#define LMS_HTTP_SERVER_CONN_HPP

#include "lms_conn_base.hpp"
#include "DHttpParser.hpp"
#include "DHttpHeader.hpp"
#include "rtmp_global.hpp"
#include "lms_http_stream_live.hpp"
#include "lms_http_stream_recv.hpp"
#include "lms_http_send_file.hpp"

class lms_source;

class lms_http_server_conn : public lms_conn_base
{
public:
    lms_http_server_conn(DThread *parent, DEvent *ev, int fd);
    virtual ~lms_http_server_conn();

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
    int read_header();
    int parse_header();

    int process_get(const DString &_url);
    int process_post(const DString &_url);

    int start_send_file();
    int start_stream_live();

    int start_stream_recv();

    int response_http_refuse(int err, const DString &info);

private:
    DHttpParser *m_parser;
    dint8 m_schedule;

    lms_http_stream_live *m_stream_live;
    lms_http_stream_recv *m_stream_recv;

    lms_http_send_file *m_sendfile;
};

#endif // LMS_HTTP_SERVER_CONN_HPP
