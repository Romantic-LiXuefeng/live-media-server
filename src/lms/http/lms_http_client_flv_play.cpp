#include "lms_http_client_flv_play.hpp"
#include "lms_edge.hpp"
#include "DHostInfo.hpp"
#include "kernel_request.hpp"
#include "kernel_log.hpp"
#include "kernel_errno.hpp"
#include "lms_config.hpp"
#include "lms_source.hpp"
#include "DHttpHeader.hpp"

lms_http_client_flv_play::lms_http_client_flv_play(lms_edge *parent, DEvent *event, lms_source *source)
    : DTcpSocket(event)
    , m_parent(parent)
    , m_source(source)
    , m_src_req(NULL)
    , m_dst_req(NULL)
    , m_flv_reader(NULL)
    , m_timeout(30 * 1000 * 1000)
    , m_started(false)
{
    m_reader = new http_reader(this, false, HTTP_HEADER_CALLBACK(&lms_http_client_flv_play::onHttpParser));

    setWriteTimeOut(m_timeout);
    setReadTimeOut(m_timeout);
}

lms_http_client_flv_play::~lms_http_client_flv_play()
{
    DFree(m_reader);
    DFree(m_flv_reader);
}

void lms_http_client_flv_play::start(kernel_request *src, kernel_request *dst, const DString &host, int port)
{
    m_src_req = src;
    m_dst_req = dst;
    m_port = port;

    get_config_value();

    log_info("flv play client start. host=%s, port=%d", host.c_str(), port);

    DHostInfo *h = new DHostInfo(m_event);
    h->setFinishedHandler(HOST_CALLBACK(&lms_http_client_flv_play::onHost));
    h->setErrorHandler(HOST_CALLBACK(&lms_http_client_flv_play::onHostError));

    if (!h->lookupHost(host)) {
        log_error("lookup host failed. ret=%d", ERROR_LOOKUP_HOST);
        h->close();
        release();
    }
}

void lms_http_client_flv_play::release()
{
    global_context->update_id(m_fd);

    log_trace("flv play client released");

    m_source->stop_external();

    m_parent->release();

    global_context->delete_id(m_fd);

    close();
}

void lms_http_client_flv_play::reload()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_src_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        release();
        return;
    }

    int timeout = config->get_http_timeout(m_src_req);
    m_timeout = timeout * 1000 * 1000;

    setReadTimeOut(m_timeout);
}

int lms_http_client_flv_play::onReadProcess()
{
    int ret = ERROR_SUCCESS;

    global_context->update_id(m_fd);

    if ((ret = m_reader->service()) != ERROR_SUCCESS) {
        if (ret != SOCKET_EAGAIN) {
            log_error("flv play client read and parse http request failed. ret=%d", ret);
            return ret;
        }
    }

    if (m_flv_reader) {
        return m_flv_reader->service();
    }

    return ret;
}

int lms_http_client_flv_play::onWriteProcess()
{
    int ret = ERROR_SUCCESS;

    global_context->update_id(m_fd);

    if ((ret = do_start()) != ERROR_SUCCESS) {
        return ret;
    }

    return ret;
}

void lms_http_client_flv_play::onReadTimeOutProcess()
{
    release();
}

void lms_http_client_flv_play::onWriteTimeOutProcess()
{
    release();
}

void lms_http_client_flv_play::onErrorProcess()
{
    release();
}

void lms_http_client_flv_play::onCloseProcess()
{
    release();
}

void lms_http_client_flv_play::get_config_value()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_src_req);
    DAutoFree(lms_server_config_struct, config);

    int timeout = config->get_http_timeout(m_src_req);
    m_timeout = timeout * 1000 * 1000;
}

void lms_http_client_flv_play::onHost(const DStringList &ips)
{
    int ret = ERROR_SUCCESS;

    if (ips.empty()) {
        release();
        return;
    }

    DString ip = ips.at(0);
    log_info("flv play client lookup origin ip=[%s]", ip.c_str());

    if ((ret = connectToHost(ip.c_str(), m_port)) != ERROR_SUCCESS) {
        if (ret != SOCKET_EINPROGRESS) {
            ret = ERROR_TCP_SOCKET_CONNECT;
            log_error("flv play client connect to host failed. ip=%s, port=%d, ret=%d", ip.c_str(), m_port, ret);
            release();
        }
    }
}

void lms_http_client_flv_play::onHostError(const DStringList &ips)
{
    log_error("flv play client get host failed.");
    release();
}

int lms_http_client_flv_play::do_start()
{
    if (m_started) {
        return ERROR_SUCCESS;
    }

    global_context->set_id(m_fd);
    global_context->update_id(m_fd);

    m_started = true;

    log_trace("flv play client start");

    return send_http_header();
}

int lms_http_client_flv_play::onMessage(CommonMessage *msg)
{
    if (msg->is_audio()) {
        return m_source->onAudio(msg);
    } else if (msg->is_video()) {
        return m_source->onVideo(msg);
    } else if (msg->is_metadata()) {
        return m_source->onMetadata(msg);
    }

    return ERROR_SUCCESS;
}

int lms_http_client_flv_play::onHttpParser(DHttpParser *parser)
{
    int ret = ERROR_SUCCESS;

    duint16 status_code = parser->statusCode();

    if (status_code != 200) {
        ret = ERROR_HTTP_FLV_PULL_REJECTED;
        log_error("http response is code is rejected, status_code=%d. ret=%d", status_code, ret);
        return ret;
    }

    setWriteTimeOut(-1);
    setReadTimeOut(m_timeout);

    m_source->start_external();

    m_flv_reader = new http_flv_reader(m_reader, AV_Handler_Callback(&lms_http_client_flv_play::onMessage));

    return ret;
}

int lms_http_client_flv_play::send_http_header()
{
    int ret = ERROR_SUCCESS;

    DHttpHeader header;

    header.setConnectionKeepAlive();
    DString host = m_dst_req->vhost + ":" + DString::number(m_port);
    header.setHost(host);
    header.addValue("Accept", "*/*");

    DString uri = "/" + m_dst_req->app + "/" + m_dst_req->stream + ".flv?type=live";
    DString str = header.getRequestString("GET", uri);

    MemoryChunk *chunk = DMemPool::instance()->getMemory(str.size());
    DSharedPtr<MemoryChunk> h = DSharedPtr<MemoryChunk>(chunk);

    memcpy(h->data, str.data(), str.size());
    h->length = str.size();

    if ((ret = write(h, h->length)) != ERROR_SUCCESS) {
        if (ret != SOCKET_EAGAIN) {
            return ERROR_HTTP_SEND_REQUEST_HEADER;
        }
    }

    return ERROR_SUCCESS;
}
