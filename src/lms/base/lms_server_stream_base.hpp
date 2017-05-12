#ifndef LMS_SERVER_STREAM_BASE_HPP
#define LMS_SERVER_STREAM_BASE_HPP

#include "rtmp_global.hpp"
#include "lms_source.hpp"
#include "lms_config.hpp"

class lms_conn_base;

class lms_server_stream_base
{
public:
    lms_server_stream_base(lms_conn_base *parent);
    virtual ~lms_server_stream_base();

public:
    virtual void release() = 0;

    void reload();
    bool get_config_value(rtmp_request *req);
    int process(CommonMessage *_msg);
    int send_message();

    DString get_client_ip();

protected:
    // 使用source处理数据
    int onMessage(CommonMessage *msg);

    void clear_messages();

    int send_gop_messages(dint64 length);

    virtual void reload_self(lms_server_config_struct *config) = 0;
    virtual bool get_self_config(lms_server_config_struct *config) = 0;
    virtual int send_av_data(CommonMessage *msg);

protected:
    lms_conn_base *m_parent;
    lms_source *m_source;
    rtmp_request *m_req;
    lms_timestamp *m_jitter;

protected:
    bool m_correct;

    std::deque<CommonMessage*> m_msgs;

    // 队列最大长度，单位是时间戳的间隔，默认30秒
    dint64 m_queue_size;
    // 当前链接中缓存数据的大小
    dint64 m_cache_size;

    // fast gop enable
    bool m_fast_enable;
    // gop cache enable
    bool m_gop_enable;

    // 超时时间
    dint64 m_timeout;

    // socket缓冲区的阀值，超过此值，不在发送新数据
    dint64 m_socket_threshold;

    DString m_client_ip;
};

#endif // LMS_SERVER_STREAM_BASE_HPP
