#ifndef LMS_HTTP_STREAM_RECV_HPP
#define LMS_HTTP_STREAM_RECV_HPP

#include "lms_server_stream_base.hpp"
#include "DHttpParser.hpp"
#include "http_flv_reader.hpp"

class lms_http_server_conn;

class lms_http_stream_recv : public lms_server_stream_base
{
public:
    lms_http_stream_recv(lms_conn_base *parent);
    virtual ~lms_http_stream_recv();

    int initialize(DHttpParser *parser);
    int start();
    int do_process();

public:
    virtual void release();

protected:
    virtual void reload_self(lms_server_config_struct *config);
    virtual bool get_self_config(lms_server_config_struct *config);

private:
    int response_http_header();

private:
    http_flv_reader *m_flv;
    dint8 m_type;
    DString m_encode;
};

#endif // LMS_HTTP_STREAM_RECV_HPP
