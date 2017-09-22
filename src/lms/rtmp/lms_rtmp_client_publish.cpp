#include "lms_rtmp_client_publish.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"
#include "lms_config.hpp"
#include "lms_edge.hpp"
#include "DHostInfo.hpp"
#include "lms_source.hpp"
#include "lms_global.hpp"

lms_rtmp_client_publish::lms_rtmp_client_publish(lms_edge *parent, DEvent *event, lms_gop_cache *gop_cache)
    : DTcpSocket(event)
    , m_parent(parent)
    , m_gop_cache(gop_cache)
    , m_src_req(NULL)
    , m_dst_req(NULL)
    , m_writer(NULL)
    , m_timeout(30 * 1000 * 1000)
    , m_started(false)
{
    m_rtmp = new rtmp_client(this);
    m_rtmp->set_publish_start_handler(Rtmp_Verify_Handler_Callback(&lms_rtmp_client_publish::onPublishStart));

    setWriteTimeOut(m_timeout);
    setReadTimeOut(m_timeout);

}

lms_rtmp_client_publish::~lms_rtmp_client_publish()
{
    DFree(m_rtmp);
    DFree(m_writer);
}

void lms_rtmp_client_publish::start(kernel_request *src, kernel_request *dst, const DString &host, int port)
{
    m_src_req = src;
    m_dst_req = dst;
    m_port = port;

    get_config_value();

    log_info("rtmp publish client start. host=%s, port=%d", host.c_str(), port);

    DHostInfo *h = new DHostInfo(m_event);
    h->setFinishedHandler(HOST_CALLBACK(&lms_rtmp_client_publish::onHost));
    h->setErrorHandler(HOST_CALLBACK(&lms_rtmp_client_publish::onHostError));

    if (!h->lookupHost(host)) {
        log_error("lookup host failed. ret=%d", ERROR_LOOKUP_HOST);
        h->close();
        release();
    }
}

void lms_rtmp_client_publish::release()
{
    global_context->update_id(m_fd);

    log_trace("rtmp publish client released");

    m_parent->release();

    global_context->delete_id(m_fd);

    close();
}

void lms_rtmp_client_publish::process(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if (m_writer && ((ret = m_writer->send(msg)) != ERROR_SUCCESS)) {
        if (ret != SOCKET_EAGAIN) {
            log_error("rtmp publish client send message failed. ret=%d", ret);
            release();
        }
    }
}

void lms_rtmp_client_publish::reload()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_src_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        release();
        return;
    }

    int timeout = config->get_rtmp_timeout(m_src_req);
    m_timeout = timeout * 1000 * 1000;

    setWriteTimeOut(m_timeout);

    if (m_writer) {
        m_writer->reload();
    }
}

int lms_rtmp_client_publish::onReadProcess()
{
    int ret = ERROR_SUCCESS;

    global_context->update_id(m_fd);

    if ((ret = m_rtmp->service()) != ERROR_SUCCESS) {
        log_error_eagain(ret, ret, "rtmp publish client rtmp protocol failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int lms_rtmp_client_publish::onWriteProcess()
{
    int ret = ERROR_SUCCESS;

    global_context->update_id(m_fd);

    if ((ret = do_start()) != ERROR_SUCCESS) {
        return ret;
    }

    if (m_writer && ((ret = m_writer->flush()) != ERROR_SUCCESS)) {
        return ret;
    }

    return ret;
}

void lms_rtmp_client_publish::onReadTimeOutProcess()
{
    log_error("read timeout");
    release();
}

void lms_rtmp_client_publish::onWriteTimeOutProcess()
{
    log_error("write timeout");
    release();
}

void lms_rtmp_client_publish::onErrorProcess()
{
    log_error("socket error");
    release();
}

void lms_rtmp_client_publish::onCloseProcess()
{
    log_error("socket closed");
    release();
}

void lms_rtmp_client_publish::get_config_value()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_src_req);
    DAutoFree(lms_server_config_struct, config);

    int chunk_size = config->get_rtmp_chunk_size(m_src_req);
    int in_ack_size = config->get_rtmp_in_ack_size(m_src_req);

    int timeout = config->get_rtmp_timeout(m_src_req);
    m_timeout = timeout * 1000 * 1000;

    m_rtmp->set_chunk_size(chunk_size);
    m_rtmp->set_in_ack_size(in_ack_size);
}

void lms_rtmp_client_publish::onHost(const DStringList &ips)
{
    int ret = ERROR_SUCCESS;

    if (ips.empty()) {
        release();
        return;
    }

    DString ip = ips.at(0);
    log_info("rtmp publish client lookup origin ip=[%s]", ip.c_str());

    if ((ret = connectToHost(ip.c_str(), m_port)) != ERROR_SUCCESS) {
        if (ret != SOCKET_EINPROGRESS) {
            ret = ERROR_TCP_SOCKET_CONNECT;
            log_error("rtmp publish client connect to host failed. ip=%s, port=%d, ret=%d", ip.c_str(), m_port, ret);
            release();
        }
    }
}

void lms_rtmp_client_publish::onHostError(const DStringList &ips)
{
    log_error("rtmp publish client get host failed.");
    release();
}

int lms_rtmp_client_publish::do_start()
{
    if (m_started) {
        return ERROR_SUCCESS;
    }

    global_context->set_id(m_fd);
    global_context->update_id(m_fd);

    m_started = true;

    log_trace("rtmp publish client start rtmp protocol");

    return m_rtmp->start_rtmp(true, m_dst_req);
}

int lms_rtmp_client_publish::onSendMessage(CommonMessage *msg)
{
    RtmpMessage *_msg = common_to_rtmp(msg);
    DAutoFree(RtmpMessage, _msg);

    return m_rtmp->send_av_data(_msg);
}

bool lms_rtmp_client_publish::onPublishStart(kernel_request *req)
{
    int ret = ERROR_SUCCESS;

    setReadTimeOut(-1);
    setWriteTimeOut(m_timeout);

    m_writer = new lms_stream_writer(m_src_req, Rtmp_AV_Handler_Callback(&lms_rtmp_client_publish::onSendMessage), true);

    if ((ret = m_writer->send_gop_messages(m_gop_cache, 0)) != ERROR_SUCCESS) {
        if (ret != SOCKET_EAGAIN) {
            return false;
        }
    }

    return true;
}
