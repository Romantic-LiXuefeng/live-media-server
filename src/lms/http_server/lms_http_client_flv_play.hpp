#ifndef LMS_HTTP_CLIENT_FLV_PLAY_HPP
#define LMS_HTTP_CLIENT_FLV_PLAY_HPP

#include "lms_client_stream_base.hpp"
#include "http_flv_reader.hpp"
#include "DHttpHeader.hpp"
#include "DHttpParser.hpp"

class lms_http_client_flv_play : public lms_client_stream_base
{
public:
    lms_http_client_flv_play(DEvent *event, lms_source *source);
    virtual ~lms_http_client_flv_play();

protected:
    virtual int start_handler();
    virtual int do_process();

private:
    int send_http_header();
    int read_header();

private:
    http_flv_reader *m_flv;
    DHttpParser *m_parser;
    bool m_http_header;

};

#endif // LMS_HTTP_CLIENT_FLV_PLAY_HPP
