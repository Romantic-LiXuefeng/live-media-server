#ifndef LMS_HTTP_TS_RECV_HPP
#define LMS_HTTP_TS_RECV_HPP

#include "DHttpParser.hpp"
#include "lms_source.hpp"
#include "kernel_request.hpp"
#include "http_reader.hpp"
#include "codec.h"
#include "lms_http_process_base.hpp"

class lms_http_server_conn;

class lms_http_ts_recv : public lms_http_process_base
{
public:
    lms_http_ts_recv(lms_http_server_conn *conn);
    ~lms_http_ts_recv();

    int initialize(DHttpParser *parser);
    int start();

    int service();

    bool reload();
    void release();

    kernel_request *request() { return m_req; }

private:
    void get_config_value();
    int response_http_header();

    int onRecvMessage(TsDemuxerPacket pkt);

    int create_aac_message(TsDemuxerPacket &pkt);
    int create_mp3_message(TsDemuxerPacket &pkt);
    int create_avc_message(TsDemuxerPacket &pkt);
    int create_hevc_message(TsDemuxerPacket &pkt);
    int process_message(CommonMessage *msg);

private:
    lms_http_server_conn *m_conn;
    kernel_request *m_req;
    lms_source *m_source;
    CodecTsDemuxer *m_demuxer;
    http_reader *m_reader;

    bool m_enable;
    bool m_is_edge;
    dint64 m_timeout;

    int m_pkt_size;
};

#endif // LMS_HTTP_TS_RECV_HPP
