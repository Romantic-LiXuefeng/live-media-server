#ifndef LMS_HTTP_FLV_LIVE_HPP
#define LMS_HTTP_FLV_LIVE_HPP

#include "lms_stream_writer.hpp"
#include "kernel_request.hpp"
#include "DHttpParser.hpp"
#include "lms_source.hpp"
#include "http_flv_writer.hpp"

class lms_http_server_conn;

class lms_http_flv_live
{
public:
    lms_http_flv_live(lms_http_server_conn *conn);
    ~lms_http_flv_live();

    int initialize(DHttpParser *parser);
    int start();

    int flush();

    int process(CommonMessage *msg);
    bool reload();
    void release();

private:
    void get_config_value();
    int response_http_header();
    int response_flv_header();

    int onSendMessage(CommonMessage *msg);

private:
    lms_http_server_conn *m_conn;
    lms_stream_writer *m_writer;
    http_flv_writer *m_flv;
    kernel_request *m_req;
    lms_source *m_source;

    bool m_enable;
    bool m_chunked;
    dint64 m_player_buffer_length;
    bool m_is_edge;
    dint64 m_timeout;
};

#endif // LMS_HTTP_FLV_LIVE_HPP
