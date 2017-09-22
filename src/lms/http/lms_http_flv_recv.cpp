#include "lms_http_flv_recv.hpp"
#include "kernel_errno.hpp"
#include "lms_http_utility.hpp"
#include "kernel_log.hpp"
#include "lms_config.hpp"
#include "lms_http_server_conn.hpp"
#include "DHttpHeader.hpp"
#include "lms_global.hpp"

lms_http_flv_recv::lms_http_flv_recv(lms_http_server_conn *conn)
    : m_conn(conn)
    , m_flv(NULL)
    , m_req(NULL)
    , m_source(NULL)
    , m_enable(false)
    , m_timeout(10 * 1000 * 1000)
{

}

lms_http_flv_recv::~lms_http_flv_recv()
{
    DFree(m_flv);
    DFree(m_req);
}

int lms_http_flv_recv::initialize(DHttpParser *parser)
{
    int ret = ERROR_SUCCESS;

    m_req = get_http_request(parser);
    if (m_req == NULL) {
        ret = ERROR_HTTP_GENERATE_REQUEST;
        log_error("generate http request failed. ret=%d", ret);
        return ret;
    }

    log_trace("http recv. vhost=%s, app=%s, stream=%s", m_req->vhost.c_str(), m_req->app.c_str(), m_req->stream.c_str());

    get_config_value();

    if (!m_enable) {
        ret = ERROR_HTTP_FLV_RECV_REJECT;
        return ret;
    }

    m_source = lms_source_manager::instance()->addSource(m_req);

    if (!m_source->add_reload_conn(m_conn)) {
        ret = ERROR_SOURCE_ADD_RELOAD;
        return ret;
    }

    if (!m_source->onPublish(m_conn->getEvent(), m_is_edge)) {
        ret = ERROR_SOURCE_ONPUBLISH;
        return ret;
    }

    m_source->start_external();

    m_flv = new http_flv_reader(m_conn->reader(), AV_Handler_Callback(&lms_http_flv_recv::onRecvMessage));

    return ret;
}

int lms_http_flv_recv::start()
{
    int ret = ERROR_SUCCESS;

    m_conn->setReadTimeOut(m_timeout);
    m_conn->setWriteTimeOut(-1);

    if ((ret = response_http_header()) != ERROR_SUCCESS) {
        if (ret != SOCKET_EAGAIN) {
            return ret;
        }
    }

    return ERROR_SUCCESS;
}

int lms_http_flv_recv::service()
{
    if (m_flv) {
        return m_flv->service();
    }

    return ERROR_SUCCESS;
}

bool lms_http_flv_recv::reload()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        return false;
    }

    m_enable = config->get_flv_recv_enable(m_req);
    if (!m_enable) {
        return false;
    }

    int timeout = config->get_http_timeout(m_req);
    m_timeout = timeout * 1000 * 1000;
    m_conn->setReadTimeOut(m_timeout);

    return true;
}

void lms_http_flv_recv::release()
{
    if (m_source) {
        m_source->stop_external();
        m_source->onUnpublish();
        m_source->del_reload_conn(m_conn);
    }
}

void lms_http_flv_recv::get_config_value()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        return;
    }

    m_enable = config->get_flv_recv_enable(m_req);
    m_is_edge = config->get_proxy_enable(m_req);

    int timeout = config->get_http_timeout(m_req);
    m_timeout = timeout * 1000 * 1000;
}

int lms_http_flv_recv::response_http_header()
{
    int ret = ERROR_SUCCESS;

    DHttpHeader header;
    header.setServer(LMS_VERSION);

    DString str = header.getResponseString(200);

    MemoryChunk *chunk = DMemPool::instance()->getMemory(str.size());
    DSharedPtr<MemoryChunk> response = DSharedPtr<MemoryChunk>(chunk);

    memcpy(response->data, str.data(), str.size());
    response->length = str.size();

    if ((ret = m_conn->write(response, str.size())) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_HTTP_SEND_RESPONSE_HEADER, "flv recv write http header failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int lms_http_flv_recv::onRecvMessage(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if (m_is_edge) {
        return m_source->proxyMessage(msg);
    } else if (msg->is_audio()) {
        return m_source->onAudio(msg);
    } else if (msg->is_video()) {
        return m_source->onVideo(msg);
    } else {
        return m_source->onMetadata(msg);
    }

    return ret;
}
