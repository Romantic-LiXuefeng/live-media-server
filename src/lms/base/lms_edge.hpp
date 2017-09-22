#ifndef LMS_EDGE_HPP
#define LMS_EDGE_HPP

#include "DEvent.hpp"
#include "kernel_request.hpp"
#include "kernel_global.hpp"
#include "lms_rtmp_client_publish.hpp"
#include "lms_http_client_flv_publish.hpp"
#include "lms_rtmp_client_play.hpp"
#include "lms_http_client_flv_play.hpp"
#include "lms_gop_cache.hpp"
#include "lms_http_client_ts_play.hpp"
#include "lms_http_client_ts_publish.hpp"

class lms_source;

class lms_edge : public EventTimeOutBase
{
public:
    lms_edge(lms_source *source, DEvent *event, bool publish);
    virtual ~lms_edge();

    int start(kernel_request *req);
    void stop();
    void release();
    void reload();

    void process(CommonMessage *msg);

public:
    virtual void onReadTimeOut();
    virtual void onWriteTimeOut();

private:
    bool get_config_value();
    void get_ip_port();

    void retry();

    void start_flv();
    void start_rtmp();
    void start_ts();

private:
    DEvent *m_event;
    lms_source *m_source;
    bool m_publish;
    kernel_request *m_src_req;
    kernel_request *m_dst_req;

    lms_rtmp_client_publish *m_rtmp_publish;
    lms_http_client_flv_publish *m_flv_publish;
    lms_gop_cache *m_gop_cache;

    lms_rtmp_client_play *m_rtmp_play;
    lms_http_client_flv_play *m_flv_play;

    lms_http_client_ts_play *m_ts_play;

    lms_http_client_ts_publish *m_ts_publish;

private:
    std::vector<DString> m_proxy_pass;

    enum ProxyType
    {
        Rtmp = 0,
        Flv,
        Ts
    };
    dint8 m_type;

    bool m_started;

    int m_pos;
    DString m_host;
    DString m_ip;
    int m_port;
};

#endif // LMS_EDGE_HPP
