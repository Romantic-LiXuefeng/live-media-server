#ifndef LMS_RTMP_SERVER_STREAM_HPP
#define LMS_RTMP_SERVER_STREAM_HPP

#include "lms_server_stream_base.hpp"
#include "rtmp_server.hpp"
#include "lms_verify_hooks.hpp"

class lms_rtmp_server_stream : public lms_server_stream_base
{
public:
    lms_rtmp_server_stream(lms_conn_base *parent);
    virtual ~lms_rtmp_server_stream();

    int do_process();

public:
    virtual void release();

protected:
    virtual void reload_self(lms_server_config_struct *config);
    virtual bool get_self_config(lms_server_config_struct *config);
    virtual int send_av_data(CommonMessage *msg);

private:
    /**
     * @brief 验证connect
     */
    void onConnect(rtmp_request *req);
    /**
     * @brief 验证play
     */
    void onPlay(rtmp_request *req);
    /**
     * @brief 验证publish
     */
    void onPublish(rtmp_request *req);
    /**
     * @brief 交互包完成，开始发送数据，主要用于metadata、sequence包及gop等包的发送
     */
    bool onPlayStart(rtmp_request *req);

private:
    void onVerifyConnectFinished(bool value);
    void onVerifyPublishFinished(bool value);
    void onVerifyPlayFinished(bool value);

private:
    rtmp_server *m_rtmp;

    enum operate_type
    {
        rtmp_default = 0,
        rtmp_play,
        rtmp_publish
    };
    int m_type;

private:
    DEvent *m_event;

    DString m_publish_pattern;
    DString m_publish_url;

    DString m_play_pattern;
    DString m_play_url;

    DString m_unpublish_url;
    DString m_stop_url;

    dint64 m_hook_timeout;
};

#endif // LMS_RTMP_SERVER_STREAM_HPP
