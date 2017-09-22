#ifndef LMS_RTMP_CLIENT_PLAY_HPP
#define LMS_RTMP_CLIENT_PLAY_HPP

#include "DTcpSocket.hpp"
#include "rtmp_client.hpp"

class kernel_request;
class lms_edge;
class lms_source;

class lms_rtmp_client_play : public DTcpSocket
{
public:
    lms_rtmp_client_play(lms_edge *parent, DEvent *event, lms_source *source);
    virtual ~lms_rtmp_client_play();

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

    int do_start();
    int onMessage(RtmpMessage *msg);
    bool onPlayStart(kernel_request *req);

private:
    lms_edge *m_parent;
    lms_source *m_source;

    kernel_request *m_src_req;
    kernel_request *m_dst_req;

    rtmp_client *m_rtmp;

private:
    int m_port;
    dint64 m_timeout;

    bool m_started;

};

#endif // LMS_RTMP_CLIENT_PLAY_HPP
