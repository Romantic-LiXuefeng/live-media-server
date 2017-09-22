#ifndef LMS_RTMP_CLIENT_PUBLISH_HPP
#define LMS_RTMP_CLIENT_PUBLISH_HPP

#include "DTcpSocket.hpp"
#include "kernel_request.hpp"
#include "rtmp_client.hpp"
#include "lms_stream_writer.hpp"
#include "lms_gop_cache.hpp"

class lms_source;
class lms_edge;

class lms_rtmp_client_publish : public DTcpSocket
{
public:
    lms_rtmp_client_publish(lms_edge *parent, DEvent *event, lms_gop_cache *gop_cache);
    virtual ~lms_rtmp_client_publish();

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
    bool onPublishStart(kernel_request *req);

private:
    lms_edge *m_parent;
    lms_gop_cache *m_gop_cache;
    kernel_request *m_src_req;
    kernel_request *m_dst_req;

    rtmp_client *m_rtmp;
    lms_stream_writer *m_writer;


private:
    int m_port;
    dint64 m_timeout;

    bool m_started;
};

#endif // LMS_RTMP_CLIENT_PUBLISH_HPP
