#include "lms_client_stream_base.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"
#include "lms_source.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

lms_client_stream_base::lms_client_stream_base(DEvent *event, lms_source *source)
    : DTcpSocket(event)
    , m_source(source)
    , m_req(NULL)
    , m_release_handler(NULL)
    , m_started(false)
    , m_timeout(10 * 1000 * 1000)
    , m_buffer_size(30 * 1024 * 1024)
    , m_cache_size(0)
    , m_socket_threshold(10 * 1024 * 1024)
    , m_released(false)
{

}

lms_client_stream_base::~lms_client_stream_base()
{
    clear_messages();
}

void lms_client_stream_base::start(rtmp_request *req, const DString &host, int port)
{
    m_req = req;
    m_port = port;

    DHostInfo *h = new DHostInfo(m_event);
    h->setFinishedHandler(HOST_CALLBACK(&lms_client_stream_base::onHost));
    h->lookupHost(host);
}

void lms_client_stream_base::set_timeout(dint64 timeout)
{
    m_timeout = timeout;

    setRecvTimeOut(m_timeout);
    setSendTimeOut(m_timeout);
}

void lms_client_stream_base::set_buffer_size(dint64 size)
{
    m_buffer_size = size;
    m_socket_threshold = DMin(m_buffer_size, 10 * 1024 * 1024);
}

void lms_client_stream_base::process(CommonMessage *msg)
{
    if (!msg) {
        return;
    }

    if (m_cache_size + getWriteBufferLen() >= m_buffer_size) {
        clear_messages();
    }

    m_cache_size += msg->payload->length;

    CommonMessage *_msg = new CommonMessage(msg);
    m_msgs.push_back(_msg);

    if (send_message() != ERROR_SUCCESS) {
        release();
    }
}

void lms_client_stream_base::setReleaseHandler(ReleaseHandler handler)
{
    m_release_handler = handler;
}

void lms_client_stream_base::release()
{
    if (m_released) {
        return;
    }

    global_context->delete_id(m_fd);

    closeSocket();

    if (m_release_handler) {
        m_release_handler();
    }

    m_released = true;
}

int lms_client_stream_base::onReadProcess()
{
    int ret = ERROR_SUCCESS;

    global_context->update_id(m_fd);

    if ((ret = do_process()) != ERROR_SUCCESS) {
        log_error("stream client do process failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int lms_client_stream_base::onWriteProcess()
{
    int ret = ERROR_SUCCESS;

    global_context->update_id(m_fd);

    if ((ret = start_process()) != ERROR_SUCCESS) {
        log_error("stream client start process failed. ret=%d", ret);
        return ret;
    }

    if ((ret = send_message()) != ERROR_SUCCESS) {
        log_error("stream client write failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

void lms_client_stream_base::onReadTimeOutProcess()
{
    release();
}

void lms_client_stream_base::onWriteTimeOutProcess()
{
    release();
}

void lms_client_stream_base::onErrorProcess()
{
    release();
}

void lms_client_stream_base::onCloseProcess()
{
    release();
}

void lms_client_stream_base::onHost(const DStringList &ips)
{
    if (ips.empty()) {
        release();
    }

    DString ip = ips.at(0);

    int ret = connectToHost(ip.c_str(), m_port);
    if (ret == EINPROGRESS) {
        return;
    } else if (ret != ERROR_SUCCESS) {
        ret = ERROR_TCP_SOCKET_CONNECT;
        log_error("stream client connect to host failed. ip=%s, port=%d, ret=%d", ip.c_str(), m_port, ret);
        release();
    } else {
        if ((ret = start_process()) != ERROR_SUCCESS) {
            log_error("stream client start process failed. ret=%d", ret);
            release();
        }

        if ((ret = writeData()) != ERROR_SUCCESS) {
            ret = ERROR_TCP_SOCKET_WRITE_FAILED;
            log_error("stream client write data failed. ret=%d", ret);
            release();
        }
    }
}

int lms_client_stream_base::onMessage(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if (!m_source) {
        return ret;
    }

    if (msg->is_audio()) {
        return m_source->onAudio(msg);
    } else if (msg->is_video()) {
        return m_source->onVideo(msg);
    } else {
        return m_source->onMetadata(msg);
    }

    return ret;
}

int lms_client_stream_base::start_process()
{
    int ret = ERROR_SUCCESS;

    if (m_started) {
        return ret;
    }

    global_context->set_id(m_fd);
    global_context->update_id(m_fd);

    if ((ret = start_handler()) != ERROR_SUCCESS) {
        log_error("stream client start failed. ret=%d", ret);
        return ret;
    }

    m_started = true;

    return ret;
}

int lms_client_stream_base::send_message()
{
    int ret = ERROR_SUCCESS;

    if (can_publish() && (getWriteBufferLen() < m_socket_threshold)) {
        for (int i = 0; i < (int)m_msgs.size(); ++i) {
            CommonMessage *msg = m_msgs.at(i);

            if ((ret = send_av_data(msg)) != ERROR_SUCCESS) {
                return ret;
            }
        }

        clear_messages();
    }

    if ((ret = writeData()) != ERROR_SUCCESS) {
        ret = ERROR_TCP_SOCKET_WRITE_FAILED;
        return ret;
    }

    return ret;
}

void lms_client_stream_base::clear_messages()
{
    for (int i = 0; i < (int)m_msgs.size(); ++i) {
        DFree(m_msgs.at(i));
    }
    m_msgs.clear();

    m_cache_size = 0;
}

int lms_client_stream_base::send_av_data(CommonMessage *msg)
{
    return ERROR_SUCCESS;
}

bool lms_client_stream_base::can_publish()
{
    return false;
}
