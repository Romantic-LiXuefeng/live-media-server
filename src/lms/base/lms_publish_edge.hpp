#ifndef LMS_PUBLISH_EDGE_HPP
#define LMS_PUBLISH_EDGE_HPP

#include "DEvent.hpp"
#include "lms_rtmp_client_publish.hpp"
#include "lms_gop_cache.hpp"
#include "lms_http_client_flv_publish.hpp"

#include <vector>

class lms_source;
class rtmp_request;

class lms_publish_edge : public EventTimeOutBase
{
public:
    lms_publish_edge(lms_source *source, DEvent *event);
    ~lms_publish_edge();

    int start(rtmp_request *req);
    void stop();

    void reload();

    void process(CommonMessage *_msg);

public:
    virtual void onReadTimeOut();
    virtual void onWriteTimeOut();

private:
    int get_config_value(rtmp_request *req);

    void retry();

    void dump_gop_messages();

    void start_flv(const DString &ip, int port);
    void start_rtmp(const DString &ip, int port);

    void onRelease();

    bool get_ip_port(DString &ip, int &port);

private:
    lms_source *m_source;
    rtmp_request *m_req;
    rtmp_request *m_base_req;
    DEvent *m_event;
    lms_gop_cache *m_gop_cache;

    lms_rtmp_client_publish *m_rtmp_publish;
    lms_http_client_flv_publish *m_flv_publish;

private:
    std::vector<DString> m_proxy_pass;
    int m_pass_pos;

    DString m_proxy_type;

    bool m_started;

    // 配置文件中配置的缓冲区大小
    dint64 m_buffer_size;

    dint64 m_timeout;

    bool m_http_chunked;
};

#endif // LMS_PUBLISH_EDGE_HPP
