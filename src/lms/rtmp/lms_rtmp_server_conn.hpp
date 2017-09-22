#ifndef LMS_RTMP_SERVER_CONN_HPP
#define LMS_RTMP_SERVER_CONN_HPP

#include "lms_conn_base.hpp"
#include "DThread.hpp"
#include "rtmp_server.hpp"
#include "lms_stream_writer.hpp"
#include "kernel_global.hpp"
#include "lms_global.hpp"

class lms_source;

/**
 * @brief 此类不需要释放，调用release函数即可
 */
class lms_rtmp_server_conn : public lms_conn_base
{
public:
    lms_rtmp_server_conn(DThread *parent, DEvent *ev, int fd);
    virtual ~lms_rtmp_server_conn();

public:
    // Inherited from DTcpSocket
    virtual int  onReadProcess();
    virtual int  onWriteProcess();
    virtual void onReadTimeOutProcess();
    virtual void onWriteTimeOutProcess();
    virtual void onErrorProcess();
    virtual void onCloseProcess();

public:
    // Inherited from lms_conn_base
    virtual int Process(CommonMessage *msg);
    virtual void reload();
    virtual void release();

private:
    void onConnect(kernel_request *req);
    void onPlay(kernel_request *req);
    void onPublish(kernel_request *req);
    bool onPlayStart(kernel_request *req);
    int onMessage(RtmpMessage *msg);

    int onSendMessage(CommonMessage *msg);

private:
    void verify_connect(kernel_request *req);
    void onVerifyConnectFinished(bool value);

    void verify_publish(kernel_request *req);
    void onVerifyPublishFinished(bool value);

    void verify_play(kernel_request *req);
    void onVerifyPlayFinished(bool value);

private:
    void get_config_value();

private:
    rtmp_server *m_rtmp;
    kernel_request *m_req;
    lms_source *m_source;
    lms_stream_writer *m_writer;

    dint64 m_hook_timeout;

    DString m_client_ip;

    DString m_publish_pattern;
    DString m_publish_url;

    DString m_play_pattern;
    DString m_play_url;

    DString m_unpublish_url;
    DString m_stop_url;

    dint64 m_timeout;
    bool m_enable;
    bool m_is_edge;

    enum operate_type
    {
        Default = 0,
        Connect,
        Play,
        Publish
    };
    dint8 m_type;

    //控制hook和release
    bool m_hooking;
    bool m_need_release;
};

#endif // LMS_RTMP_SERVER_CONN_HPP
