#ifndef LMS_PLAY_EDGE_HPP
#define LMS_PLAY_EDGE_HPP

#include "DEvent.hpp"
#include "lms_rtmp_client_play.hpp"
#include "lms_http_client_flv_play.hpp"

#include <vector>

class lms_source;
class rtmp_request;

class lms_play_edge : public EventTimeOutBase
{
public:
    lms_play_edge(lms_source *source, DEvent *event);
    ~lms_play_edge();

    int start(rtmp_request *req);
    void stop();

    void reload();

public:
    virtual void onReadTimeOut();
    virtual void onWriteTimeOut();

private:
    int get_config_value(rtmp_request *req);
    void retry();
    void onRelease();

    void start_flv(const DString &ip, int port);
    void start_rtmp(const DString &ip, int port);

    bool get_ip_port(DString &ip, int &port);

private:
    lms_source *m_source;
    rtmp_request *m_req;
    rtmp_request *m_base_req;
    DEvent *m_event;

    lms_rtmp_client_play *m_rtmp_play;
    lms_http_client_flv_play *m_flv_play;

private:
    std::vector<DString> m_proxy_pass;
    int m_pass_pos;

    DString m_proxy_type;

    bool m_started;

    dint64 m_timeout;
};

#endif // LMS_PLAY_EDGE_HPP
