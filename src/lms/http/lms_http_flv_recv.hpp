#ifndef LMS_HTTP_FLV_RECV_HPP
#define LMS_HTTP_FLV_RECV_HPP

#include "DHttpParser.hpp"
#include "lms_source.hpp"
#include "kernel_request.hpp"
#include "http_flv_reader.hpp"

class lms_http_server_conn;

class lms_http_flv_recv
{
public:
    lms_http_flv_recv(lms_http_server_conn *conn);
    ~lms_http_flv_recv();

    int initialize(DHttpParser *parser);
    int start();

    int service();

    bool reload();
    void release();

private:
    void get_config_value();
    int response_http_header();

    int onRecvMessage(CommonMessage *msg);

private:
    lms_http_server_conn *m_conn;
    http_flv_reader *m_flv;
    kernel_request *m_req;
    lms_source *m_source;

    bool m_enable;
    bool m_is_edge;
    dint64 m_timeout;

};

#endif // LMS_HTTP_FLV_RECV_HPP
