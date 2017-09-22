#ifndef LMS_HTTP_TS_LIVE_HPP
#define LMS_HTTP_TS_LIVE_HPP

#include "lms_stream_writer.hpp"
#include "kernel_request.hpp"
#include "DHttpParser.hpp"
#include "lms_source.hpp"
#include "codec.h"

class lms_http_server_conn;

class lms_http_ts_live
{
public:
    lms_http_ts_live(lms_http_server_conn *conn);
    ~lms_http_ts_live();

    int initialize(DHttpParser *parser);
    int start();

    int flush();

    int process(CommonMessage *msg);
    bool reload();
    void release();

private:
    void get_config_value();
    int response_http_header();

    int onSendMessage(CommonMessage *msg);
    int onWriteFrame(unsigned char *p, unsigned int size);

    void init_muxer();

private:
    lms_http_server_conn *m_conn;
    lms_stream_writer *m_writer;

    kernel_request *m_req;
    lms_source *m_source;

    CodecTsMuxer *m_muxer;

    bool m_enable;
    DString m_acodec;
    DString m_vcodec;
    bool m_chunked;
    dint64 m_player_buffer_length;
    bool m_is_edge;
    dint64 m_timeout;

    bool m_has_video;
    bool m_has_audio;

};

#endif // LMS_HTTP_TS_LIVE_HPP
