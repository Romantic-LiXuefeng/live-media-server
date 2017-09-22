#include "lms_http_client_flv_publish.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"
#include "lms_config.hpp"
#include "lms_edge.hpp"
#include "DHostInfo.hpp"
#include "lms_source.hpp"
#include "DHttpHeader.hpp"
#include "lms_global.hpp"

lms_http_client_flv_publish::lms_http_client_flv_publish(lms_edge *parent, DEvent *event, lms_gop_cache *gop_cache)
    : DTcpSocket(event)
    , m_parent(parent)
    , m_gop_cache(gop_cache)
    , m_src_req(NULL)
    , m_dst_req(NULL)
    , m_writer(NULL)
    , m_flv(NULL)
    , m_timeout(30 * 1000 * 1000)
    , m_started(false)
    , m_chunked(true)
{
    setWriteTimeOut(m_timeout);
    setReadTimeOut(m_timeout);

}

lms_http_client_flv_publish::~lms_http_client_flv_publish()
{
    DFree(m_writer);
    DFree(m_flv);
}

void lms_http_client_flv_publish::start(kernel_request *src, kernel_request *dst, const DString &host, int port)
{
    m_src_req = src;
    m_dst_req = dst;
    m_port = port;

    get_config_value();

    log_info("flv publish client start. host=%s, port=%d", host.c_str(), port);

    DHostInfo *h = new DHostInfo(m_event);
    h->setFinishedHandler(HOST_CALLBACK(&lms_http_client_flv_publish::onHost));
    h->setErrorHandler(HOST_CALLBACK(&lms_http_client_flv_publish::onHostError));

    if (!h->lookupHost(host)) {
        log_error("flv publish client lookup host failed. ret=%d", ERROR_LOOKUP_HOST);
        h->close();
        release();
    }
}

void lms_http_client_flv_publish::release()
{
    global_context->update_id(m_fd);

    log_trace("flv publish client released");

    m_parent->release();

    global_context->delete_id(m_fd);

    close();
}

void lms_http_client_flv_publish::process(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if (m_writer && ((ret = m_writer->send(msg)) != ERROR_SUCCESS)) {
        if (ret != SOCKET_EAGAIN) {
            log_error("flv publish client send message failed. ret=%d", ret);
            release();
        }
    }
}

void lms_http_client_flv_publish::reload()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_src_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        release();
        return;
    }

    int timeout = config->get_http_timeout(m_src_req);
    m_timeout = timeout * 1000 * 1000;

    setReadTimeOut(-1);
    setWriteTimeOut(m_timeout);

    if (m_writer) {
        m_writer->reload();
    }
}

int lms_http_client_flv_publish::onReadProcess()
{
    int len = getReadBufferLength();
    if (len > 0) {
        MemoryChunk *chunk = DMemPool::instance()->getMemory(len);
        DSharedPtr<MemoryChunk> payload = DSharedPtr<MemoryChunk>(chunk);
        read(payload->data, len);
    }

    return ERROR_SUCCESS;
}

int lms_http_client_flv_publish::onWriteProcess()
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

void lms_http_client_flv_publish::onReadTimeOutProcess()
{
    log_error("read timeout");
    release();
}

void lms_http_client_flv_publish::onWriteTimeOutProcess()
{
    log_error("write timeout");
    release();
}

void lms_http_client_flv_publish::onErrorProcess()
{
    log_error("socket error");
    release();
}

void lms_http_client_flv_publish::onCloseProcess()
{
    log_error("socket closed");
    release();
}

void lms_http_client_flv_publish::get_config_value()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_src_req);
    DAutoFree(lms_server_config_struct, config);

    int timeout = config->get_http_timeout(m_src_req);
    m_timeout = timeout * 1000 * 1000;

    m_chunked = config->get_http_chunked(m_src_req);
}

void lms_http_client_flv_publish::onHost(const DStringList &ips)
{
    int ret = ERROR_SUCCESS;

    if (ips.empty()) {
        release();
        return;
    }

    DString ip = ips.at(0);
    log_info("flv publish client lookup origin ip=[%s]", ip.c_str());

    if ((ret = connectToHost(ip.c_str(), m_port)) != ERROR_SUCCESS) {
        if (ret != SOCKET_EINPROGRESS) {
            ret = ERROR_TCP_SOCKET_CONNECT;
            log_error("flv publish client connect to host failed. ip=%s, port=%d, ret=%d", ip.c_str(), m_port, ret);
            release();
        }
    }
}

void lms_http_client_flv_publish::onHostError(const DStringList &ips)
{
    log_error("flv publish client get host failed.");
    release();
}

int lms_http_client_flv_publish::do_start()
{
    int ret = ERROR_SUCCESS;

    if (m_started) {
        return ERROR_SUCCESS;
    }

    global_context->set_id(m_fd);
    global_context->update_id(m_fd);

    m_started = true;

    setReadTimeOut(-1);
    setWriteTimeOut(m_timeout);

    if ((ret = send_http_header()) != ERROR_SUCCESS) {
        return ret;
    }

    m_flv = new http_flv_writer(this);
    m_flv->set_chunked(m_chunked);

    if ((ret = m_flv->send_flv_header()) != ERROR_SUCCESS) {
        if (ret != SOCKET_EAGAIN) {
            return ret;
        }
    }

    m_writer = new lms_stream_writer(m_src_req, AV_Handler_Callback(&lms_http_client_flv_publish::onSendMessage), true);

    if ((ret = m_writer->send_gop_messages(m_gop_cache, 0)) != ERROR_SUCCESS) {
        if (ret != SOCKET_EAGAIN) {
            return ret;
        }
    }

    return ret;
}

int lms_http_client_flv_publish::onSendMessage(CommonMessage *msg)
{
    return m_flv->send_av_data(msg);
}

int lms_http_client_flv_publish::send_http_header()
{
    int ret = ERROR_SUCCESS;

    DHttpHeader header;
    if (m_chunked) {
        header.addValue("Transfer-Encoding", "chunked");
    }

    header.setConnectionKeepAlive();
    DString host = m_dst_req->vhost + ":" + DString::number(m_port);
    header.setHost(host);

    header.addValue("Accept", "*/*");
    header.addValue("Icy-MetaData", "1");

    DString uri = "/" + m_dst_req->app + "/" + m_dst_req->stream + ".flv";
    DString str = header.getRequestString("POST", uri);

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
