#ifndef LMS_HTTP_CLIENT_FLV_PUBLISH_HPP
#define LMS_HTTP_CLIENT_FLV_PUBLISH_HPP

#include "lms_client_stream_base.hpp"
#include "http_flv_writer.hpp"
#include "DHttpHeader.hpp"
#include "DHttpParser.hpp"

#include <vector>

class lms_http_client_flv_publish : public lms_client_stream_base
{
public:
    lms_http_client_flv_publish(DEvent *event, lms_source *source);
    virtual ~lms_http_client_flv_publish();

    void set_chunked(bool chunked);

protected:
    virtual int start_handler();
    virtual int do_process();
    virtual int send_av_data(CommonMessage *msg);
    virtual bool can_publish();

private:
    int send_http_header();
    int read_header();

private:
    http_flv_writer *m_flv;
    DHttpParser *m_parser;

    bool m_chunked;
    bool m_responsed;
    bool m_can_publish;

};

#endif // LMS_HTTP_CLIENT_FLV_PUBLISH_HPP
