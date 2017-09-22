#include "lms_rtmp_client_play.hpp"
#include "lms_edge.hpp"
#include "DHostInfo.hpp"
#include "kernel_request.hpp"
#include "kernel_log.hpp"
#include "kernel_errno.hpp"
#include "lms_config.hpp"
#include "lms_source.hpp"
#include "lms_global.hpp"

lms_rtmp_client_play::lms_rtmp_client_play(lms_edge *parent, DEvent *event, lms_source *source)
    : DTcpSocket(event)
    , m_parent(parent)
    , m_source(source)
    , m_src_req(NULL)
    , m_dst_req(NULL)
    , m_timeout(30 * 1000 * 1000)
    , m_started(false)
{
    m_rtmp = new rtmp_client(this);
    m_rtmp->set_buffer_length(10000);
    m_rtmp->set_av_handler(Rtmp_AV_Handler_Callback(&lms_rtmp_client_play::onMessage));
    m_rtmp->set_metadata_handler(Rtmp_AV_Handler_Callback(&lms_rtmp_client_play::onMessage));
    m_rtmp->set_play_start_handler(Rtmp_Verify_Handler_Callback(&lms_rtmp_client_play::onPlayStart));

    setWriteTimeOut(m_timeout);
    setReadTimeOut(m_timeout);
}

lms_rtmp_client_play::~lms_rtmp_client_play()
{
    DFree(m_rtmp);
}

void lms_rtmp_client_play::start(kernel_request *src, kernel_request *dst, const DString &host, int port)
{
    m_src_req = src;
    m_dst_req = dst;
    m_port = port;

    get_config_value();

    log_info("rtmp play client start. host=%s, port=%d", host.c_str(), port);

    DHostInfo *h = new DHostInfo(m_event);
    h->setFinishedHandler(HOST_CALLBACK(&lms_rtmp_client_play::onHost));
    h->setErrorHandler(HOST_CALLBACK(&lms_rtmp_client_play::onHostError));

    if (!h->lookupHost(host)) {
        log_error("lookup host failed. ret=%d", ERROR_LOOKUP_HOST);
        h->close();
        release();
    }
}

void lms_rtmp_client_play::release()
{
    global_context->update_id(m_fd);

    log_trace("rtmp play client released");

    if (m_source) {
        m_source->stop_external();
    }

    m_parent->release();

    global_context->delete_id(m_fd);

    close();
}

void lms_rtmp_client_play::reload()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_src_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        release();
        return;
    }

    int timeout = config->get_rtmp_timeout(m_src_req);
    m_timeout = timeout * 1000 * 1000;

    setReadTimeOut(m_timeout);
}

int lms_rtmp_client_play::onReadProcess()
{
    int ret = ERROR_SUCCESS;

    global_context->update_id(m_fd);

    if ((ret != m_rtmp->service()) != ERROR_SUCCESS) {
        return ret;
    }

    return ret;
}

int lms_rtmp_client_play::onWriteProcess()
{
    int ret = ERROR_SUCCESS;

    global_context->update_id(m_fd);

    if ((ret = do_start()) != ERROR_SUCCESS) {
        return ret;
    }

    return ret;
}

void lms_rtmp_client_play::onReadTimeOutProcess()
{
    release();
}

void lms_rtmp_client_play::onWriteTimeOutProcess()
{
    release();
}

void lms_rtmp_client_play::onErrorProcess()
{
    release();
}

void lms_rtmp_client_play::onCloseProcess()
{
    release();
}

void lms_rtmp_client_play::get_config_value()
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

void lms_rtmp_client_play::onHost(const DStringList &ips)
{
    int ret = ERROR_SUCCESS;

    if (ips.empty()) {
        release();
        return;
    }

    DString ip = ips.at(0);
    log_info("rtmp play client lookup origin ip=[%s]", ip.c_str());

    if ((ret = connectToHost(ip.c_str(), m_port)) != ERROR_SUCCESS) {
        if (ret != SOCKET_EINPROGRESS) {
            ret = ERROR_TCP_SOCKET_CONNECT;
            log_error("rtmp play client connect to host failed. ip=%s, port=%d, ret=%d", ip.c_str(), m_port, ret);
            release();
        }
    }
}

void lms_rtmp_client_play::onHostError(const DStringList &ips)
{
    log_error("rtmp play client get host failed.");
    release();
}

int lms_rtmp_client_play::do_start()
{
    if (m_started) {
        return ERROR_SUCCESS;
    }

    global_context->set_id(m_fd);
    global_context->update_id(m_fd);

    m_started = true;

    log_trace("rtmp play client start rtmp protocol");

    return m_rtmp->start_rtmp(false, m_dst_req);
}

int lms_rtmp_client_play::onMessage(RtmpMessage *msg)
{
    CommonMessage *_msg = rtmp_to_common(msg);
    DAutoFree(CommonMessage, _msg);

    if (_msg->is_audio()) {
        return m_source->onAudio(_msg);
    } else if (_msg->is_video()) {
        return m_source->onVideo(_msg);
    } else if (_msg->is_metadata()){
        return m_source->onMetadata(_msg);
    }

    return ERROR_SUCCESS;
}

bool lms_rtmp_client_play::onPlayStart(kernel_request *req)
{
    setReadTimeOut(m_timeout);
    setWriteTimeOut(-1);

    m_source->start_external();

    return true;
}
