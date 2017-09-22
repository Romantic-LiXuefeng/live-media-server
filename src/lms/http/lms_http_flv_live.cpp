#include "lms_http_flv_live.hpp"
#include "kernel_errno.hpp"
#include "lms_http_utility.hpp"
#include "kernel_log.hpp"
#include "lms_config.hpp"
#include "lms_http_server_conn.hpp"
#include "DHttpHeader.hpp"
#include "lms_global.hpp"

lms_http_flv_live::lms_http_flv_live(lms_http_server_conn *conn)
    : m_conn(conn)
    , m_writer(NULL)
    , m_flv(NULL)
    , m_req(NULL)
    , m_source(NULL)
    , m_enable(false)
    , m_chunked(true)
    , m_player_buffer_length(1000)
    , m_is_edge(false)
    , m_timeout(10 * 1000 * 1000)
{

}

lms_http_flv_live::~lms_http_flv_live()
{
    DFree(m_writer);
    DFree(m_flv);
    DFree(m_req);
}

int lms_http_flv_live::initialize(DHttpParser *parser)
{
    int ret = ERROR_SUCCESS;

    m_req = get_http_request(parser);
    if (m_req == NULL) {
        ret = ERROR_HTTP_GENERATE_REQUEST;
        log_error("generate http request failed. ret=%d", ret);
        return ret;
    }

    get_config_value();

    if (!m_enable) {
        ret = ERROR_HTTP_FLV_LIVE_REJECT;
        return ret;
    }

    m_source = lms_source_manager::instance()->addSource(m_req);

    if (!m_source->add_connection(m_conn)) {
        ret = ERROR_SOURCE_ADD_CONNECTION;
        return ret;
    }

    if (!m_source->add_reload_conn(m_conn)) {
        ret = ERROR_SOURCE_ADD_RELOAD;
        return ret;
    }

    if (!m_source->onPlay(m_conn->getEvent(), m_is_edge)) {
        ret = ERROR_SOURCE_ONPLAY;
        return ret;
    }

    m_writer = new lms_stream_writer(m_req, AV_Handler_Callback(&lms_http_flv_live::onSendMessage), false);

    m_flv = new http_flv_writer(m_conn);
    m_flv->set_chunked(m_chunked);

    return ret;
}

int lms_http_flv_live::start()
{
    int ret = ERROR_SUCCESS;

    m_conn->setReadTimeOut(-1);
    m_conn->setWriteTimeOut(m_timeout);

    if ((ret = response_http_header()) != ERROR_SUCCESS) {
        if (ret != SOCKET_EAGAIN) {
            return ret;
        }
    }

    if ((ret = response_flv_header()) != ERROR_SUCCESS) {
        if (ret != SOCKET_EAGAIN) {
            return ret;
        }
    }

    if ((ret = m_source->send_gop_cache(m_writer, m_player_buffer_length)) != ERROR_SUCCESS) {
        if (ret != SOCKET_EAGAIN) {
            return ret;
        }
    }

    return ERROR_SUCCESS;
}

int lms_http_flv_live::flush()
{
    if (m_writer) {
        return m_writer->flush();
    }

    return ERROR_SUCCESS;
}

int lms_http_flv_live::process(CommonMessage *msg)
{
    if (m_writer) {
        return m_writer->send(msg);
    }

    return ERROR_SUCCESS;
}

bool lms_http_flv_live::reload()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        return false;
    }

    m_enable = config->get_flv_live_enable(m_req);
    if (!m_enable) {
        return false;
    }

    int timeout = config->get_http_timeout(m_req);
    m_timeout = timeout * 1000 * 1000;
    m_conn->setWriteTimeOut(m_timeout);

    if (m_writer) {
        m_writer->reload();
    }

    return true;
}

void lms_http_flv_live::release()
{
    if (m_source) {
        m_source->del_connection(m_conn);
        m_source->del_reload_conn(m_conn);
    }
}

void lms_http_flv_live::get_config_value()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        return;
    }

    m_enable = config->get_flv_live_enable(m_req);
    m_chunked = config->get_http_chunked(m_req);
    m_player_buffer_length = config->get_http_buffer_length(m_req);
    m_is_edge = config->get_proxy_enable(m_req);

    int timeout = config->get_http_timeout(m_req);
    m_timeout = timeout * 1000 * 1000;
}

int lms_http_flv_live::response_http_header()
{
    int ret = ERROR_SUCCESS;

    DHttpHeader header;
    header.setServer(LMS_VERSION);

    header.setContentType("flv");
    header.setConnectionKeepAlive();

    if (m_chunked) {
        header.addValue("Transfer-Encoding", "chunked");
    }

    DString str = header.getResponseString(200);

    MemoryChunk *chunk = DMemPool::instance()->getMemory(str.size());
    DSharedPtr<MemoryChunk> response = DSharedPtr<MemoryChunk>(chunk);

    memcpy(response->data, str.data(), str.size());
    response->length = str.size();

    if ((ret = m_conn->write(response, str.size())) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_HTTP_SEND_RESPONSE_HEADER, "flv live write http header failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int lms_http_flv_live::response_flv_header()
{
    return m_flv->send_flv_header();
}

int lms_http_flv_live::onSendMessage(CommonMessage *msg)
{
    return m_flv->send_av_data(msg);
}
