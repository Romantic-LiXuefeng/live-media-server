#ifndef LMS_HTTP_STREAM_LIVE_HPP
#define LMS_HTTP_STREAM_LIVE_HPP

#include "lms_server_stream_base.hpp"
#include "http_flv_writer.hpp"
#include "DHttpParser.hpp"

#include <deque>

class lms_http_stream_live : public lms_server_stream_base
{
public:
    lms_http_stream_live(lms_conn_base *parent);
    virtual ~lms_http_stream_live();

    int initialize(DHttpParser *parser);
    int start();

public:
    virtual void release();

protected:
    virtual void reload_self(lms_server_config_struct *config);
    virtual bool get_self_config(lms_server_config_struct *config);
    virtual int send_av_data(CommonMessage *msg);

private:
    int response_http_header();

private:
    http_flv_writer *m_flv;
    bool m_chunked;
    int m_buffer_length;
    dint8 m_type;

};

#endif // LMS_HTTP_STREAM_LIVE_HPP
