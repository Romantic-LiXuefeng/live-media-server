#ifndef LMS_CLIENT_STREAM_BASE_HPP
#define LMS_CLIENT_STREAM_BASE_HPP

#include "DTcpSocket.hpp"
#include "rtmp_global.hpp"
#include "DHostInfo.hpp"

typedef std::tr1::function<void (void)> ReleaseHandler;
#define RELEASE_CALLBACK(str)  std::tr1::bind(str, this)

class lms_source;

class lms_client_stream_base : public DTcpSocket
{
public:
    lms_client_stream_base(DEvent *event, lms_source *source);
    virtual ~lms_client_stream_base();

    void start(rtmp_request *req, const DString &host, int port);

    void set_timeout(dint64 timeout);
    void set_buffer_size(dint64 size);

    void process(CommonMessage *msg);

    void setReleaseHandler(ReleaseHandler handler);
    void release();

public:
    virtual int onReadProcess();
    virtual int onWriteProcess();
    virtual void onReadTimeOutProcess();
    virtual void onWriteTimeOutProcess();
    virtual void onErrorProcess();
    virtual void onCloseProcess();

protected:
    virtual int start_handler() = 0;
    virtual int do_process() = 0;
    virtual int send_av_data(CommonMessage *msg);
    virtual bool can_publish();

    int start_process();
    int send_message();
    void clear_messages();

    void onHost(const DStringList &ips);
    int onMessage(CommonMessage *msg);

protected:
    lms_source *m_source;
    rtmp_request *m_req;
    ReleaseHandler m_release_handler;

    bool m_started;
    int m_port;
    dint64 m_timeout;

    std::deque<CommonMessage*> m_msgs;

    // 配置文件中设置的缓存大小
    dint64 m_buffer_size;
    // 当前队列缓存的数据大小
    dint64 m_cache_size;
    // socket缓冲区的阀值，超过此值，不在发送新数据
    dint64 m_socket_threshold;

    bool m_released;
};

#endif // LMS_CLIENT_STREAM_BASE_HPP
