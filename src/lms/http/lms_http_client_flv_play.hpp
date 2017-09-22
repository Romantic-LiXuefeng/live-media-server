#ifndef LMS_HTTP_CLIENT_FLV_PLAY_HPP
#define LMS_HTTP_CLIENT_FLV_PLAY_HPP

#include "DTcpSocket.hpp"
#include "http_reader.hpp"
#include "kernel_global.hpp"
#include "http_flv_reader.hpp"

class kernel_request;
class lms_edge;
class lms_source;

class lms_http_client_flv_play : public DTcpSocket
{
public:
    lms_http_client_flv_play(lms_edge *parent, DEvent *event, lms_source *source);
    virtual ~lms_http_client_flv_play();

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
    int onMessage(CommonMessage *msg);

    int onHttpParser(DHttpParser *parser);

    int send_http_header();

private:
    lms_edge *m_parent;
    lms_source *m_source;

    kernel_request *m_src_req;
    kernel_request *m_dst_req;

    http_reader *m_reader;
    http_flv_reader *m_flv_reader;

private:
    int m_port;
    dint64 m_timeout;

    bool m_started;

};

#endif // LMS_HTTP_CLIENT_FLV_PLAY_HPP
