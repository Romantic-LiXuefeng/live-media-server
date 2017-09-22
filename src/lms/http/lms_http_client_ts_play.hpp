#ifndef LMS_HTTP_CLIENT_TS_PLAY_HPP
#define LMS_HTTP_CLIENT_TS_PLAY_HPP

#include "DTcpSocket.hpp"
#include "http_reader.hpp"
#include "kernel_global.hpp"
#include "codec.h"

class kernel_request;
class lms_edge;
class lms_source;

class lms_http_client_ts_play : public DTcpSocket
{
public:
    lms_http_client_ts_play(lms_edge *parent, DEvent *event, lms_source *source);
    virtual ~lms_http_client_ts_play();

    void start(kernel_request *src, kernel_request *dst, const DString &host, int port);

    void release();

    void reload();

public:
    virtual int onReadProcess();
    virtual int onWriteProcess();
    virtual void onReadTimeOutProcess();
    virtual void onWriteTimeOutProcess();
    virtual void onErrorProcess();
    virtual void onCloseProcess();

private:
    void get_config_value();
    void onHost(const DStringList &ips);
    void onHostError(const DStringList &ips);

    int onHttpParser(DHttpParser *parser);

    int send_http_header();

    int do_start();

    int onRecvMessage(TsDemuxerPacket pkt);
    int create_aac_message(TsDemuxerPacket &pkt);
    int create_mp3_message(TsDemuxerPacket &pkt);
    int create_avc_message(TsDemuxerPacket &pkt);
    int create_hevc_message(TsDemuxerPacket &pkt);
    int process_message(CommonMessage *msg);

private:
    lms_edge *m_parent;
    lms_source *m_source;

    kernel_request *m_src_req;
    kernel_request *m_dst_req;

    http_reader *m_reader;
    CodecTsDemuxer *m_demuxer;

private:
    int m_port;
    dint64 m_timeout;

    bool m_started;
    int m_pkt_size;

};

#endif // LMS_HTTP_CLIENT_TS_PLAY_HPP
