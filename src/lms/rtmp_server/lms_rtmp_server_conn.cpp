#include "lms_rtmp_server_conn.hpp"
#include "kernel_log.hpp"
#include "kernel_errno.hpp"

lms_rtmp_server_conn::lms_rtmp_server_conn(DThread *parent, DEvent *ev, int fd)
    : lms_conn_base(parent, ev, fd)
{
    m_rtmp = new lms_rtmp_server_stream(this);

    setSendTimeOut(30 * 1000 * 1000);
    setRecvTimeOut(30 * 1000 * 1000);
}

lms_rtmp_server_conn::~lms_rtmp_server_conn()
{
    log_warn("free --> lms_rtmp_server_conn");
    DFree(m_rtmp);
}

int lms_rtmp_server_conn::onReadProcess()
{
    int ret = ERROR_SUCCESS;

    global_context->update_id(m_fd);

    if ((ret = m_rtmp->do_process()) != ERROR_SUCCESS) {
        log_error("rtmp server process failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int lms_rtmp_server_conn::onWriteProcess()
{
    int ret = ERROR_SUCCESS;

    global_context->update_id(m_fd);

    if ((ret = m_rtmp->send_message()) != ERROR_SUCCESS) {
        log_error("rtmp server send message failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

void lms_rtmp_server_conn::onReadTimeOutProcess()
{
    release();
}

void lms_rtmp_server_conn::onWriteTimeOutProcess()
{
    release();
}

void lms_rtmp_server_conn::onErrorProcess()
{
    release();
}

void lms_rtmp_server_conn::onCloseProcess()
{
    release();
}

int lms_rtmp_server_conn::Process(CommonMessage *_msg)
{
    return m_rtmp->process(_msg);
}

void lms_rtmp_server_conn::reload()
{
    m_rtmp->reload();
}

void lms_rtmp_server_conn::release()
{
    global_context->update_id(m_fd);
    log_trace("rtmp connection close");

    m_rtmp->release();
    global_context->delete_id(m_fd);
    closeSocket();
}

