#ifndef LMS_HTTP_CLIENT_TS_PUBLISH_HPP
#define LMS_HTTP_CLIENT_TS_PUBLISH_HPP

#include "DTcpSocket.hpp"
#include "kernel_request.hpp"
#include "lms_stream_writer.hpp"
#include "lms_gop_cache.hpp"
#include "codec.h"

class lms_source;
class lms_edge;

class lms_http_client_ts_publish : public DTcpSocket
{
public:
    lms_http_client_ts_publish(lms_edge *parent, DEvent *event, lms_gop_cache *gop_cache);
    ~lms_http_client_ts_publish();

    void start(kernel_request *src, kernel_request *dst, const DString &host, int port);

    void release();

    void process(CommonMessage *msg);

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

    int do_start();

    int onSendMessage(CommonMessage *msg);

    int send_http_header();

    void init_muxer();
    int onWriteFrame(unsigned char *p, unsigned int size);

private:
    lms_edge *m_parent;
    lms_gop_cache *m_gop_cache;
    kernel_request *m_src_req;
    kernel_request *m_dst_req;

    lms_stream_writer *m_writer;
    CodecTsMuxer *m_muxer;

private:
    int m_port;
    dint64 m_timeout;
    bool m_started;
    bool m_chunked;

    DString m_acodec;
    DString m_vcodec;

    bool m_has_video;
    bool m_has_audio;

};

#endif // LMS_HTTP_CLIENT_TS_PUBLISH_HPP
