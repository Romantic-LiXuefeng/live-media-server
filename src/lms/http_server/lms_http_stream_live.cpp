#include "lms_http_stream_live.hpp"
#include "DHttpHeader.hpp"
#include "DMemPool.hpp"
#include "DSharedPtr.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"
#include "lms_http_utility.hpp"
#include "lms_verify_refer.hpp"

lms_http_stream_live::lms_http_stream_live(lms_conn_base *parent)
    : lms_server_stream_base(parent)
    , m_flv(NULL)
    , m_chunked(true)
    , m_buffer_length(3000)
{

}

lms_http_stream_live::~lms_http_stream_live()
{
    DFree(m_flv);
}

int lms_http_stream_live::initialize(DHttpParser *parser)
{
    int ret = ERROR_SUCCESS;

    if (!generate_http_request(parser, m_req)) {
        ret = ERROR_GENERATE_REQUEST;
        log_error("http stream live generate http request failed. ret=%d", ret);
        return ret;
    }

    if (!get_config_value(m_req)) {
        ret = ERROR_GET_CONFIG_VALUE;
        log_error("http stream live get config value failed. ret=%d", ret);
        return ret;
    }

    // 验证refer防盗链
    lms_verify_refer refer;
    if (!refer.check(m_req, false)) {
        ret = ERROR_REFERER_VERIFY;
        log_trace("http stream live referer verify refused.");
        return ret;
    }

    m_type = get_http_stream_type(parser);

    m_source = lms_source_manager::instance()->addSource(m_req);
    m_source->add_reload(dynamic_cast<lms_conn_base*>(m_parent));
    m_source->add_connection(dynamic_cast<lms_conn_base*>(m_parent));

    if (!m_source->onPlay(m_parent->getEvent())) {
        ret = ERROR_SOURCE_ONPLAY;
        log_error("http stream live source onPlay failed. ret=%d", ret);
        return ret;
    }

    log_trace("http stream live. vhost=%s, app=%s, stream=%s", m_req->vhost.c_str(), m_req->app.c_str(), m_req->stream.c_str());

    return ret;
}

int lms_http_stream_live::start()
{
    int ret = ERROR_SUCCESS;

    m_parent->setSendTimeOut(m_timeout);
    m_parent->setRecvTimeOut(-1);

    if ((ret = response_http_header()) != ERROR_SUCCESS) {
        log_error("http stream live response http header failed. ret=%d", ret);
        return ret;
    }

    if (m_type == HttpStreamFlv) {
        m_flv = new http_flv_writer(dynamic_cast<DTcpSocket*>(m_parent));
        m_flv->set_chunked(m_chunked);
        m_flv->send_flv_header();
    }

    if ((ret = send_gop_messages(m_buffer_length)) != ERROR_SUCCESS) {
        log_error("http stream live send gop messages failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

void lms_http_stream_live::release()
{
    if (m_source) {
        m_source->del_connection(dynamic_cast<lms_conn_base*>(m_parent));
        m_source->del_reload(dynamic_cast<lms_conn_base*>(m_parent));
    }

    m_source = NULL;
}

void lms_http_stream_live::reload_self(lms_server_config_struct *config)
{
    m_parent->setSendTimeOut(m_timeout);

    bool enable = config->get_http_enable(m_req);
    if (!enable) {
        m_parent->release();
    }
}

bool lms_http_stream_live::get_self_config(lms_server_config_struct *config)
{
    m_buffer_length = config->get_http_buffer_length(m_req);
    m_chunked = config->get_http_chunked(m_req);

    return config->get_http_enable(m_req);
}

int lms_http_stream_live::send_av_data(CommonMessage *msg)
{
    if (m_type == HttpStreamFlv) {
        m_flv->send_av_data(msg);
    }

    return ERROR_SUCCESS;
}

int lms_http_stream_live::response_http_header()
{
    int ret = ERROR_SUCCESS;

    DHttpHeader header;
    header.setServer("lms/1.0.0");
    header.setConnectionKeepAlive();

    if (m_type == HttpStreamFlv) {
        header.setContentType("video/x-flv");
    } else if (m_type == HttpStreamTs) {
        header.setContentType("video/mp2t");
    }

    if (m_chunked) {
        header.addValue("Transfer-Encoding", "chunked");
    }

    DString value = header.getResponseString(200, "OK");

    MemoryChunk *chunk = DMemPool::instance()->getMemory(value.size());
    DSharedPtr<MemoryChunk> response = DSharedPtr<MemoryChunk>(chunk);

    memcpy(response->data, value.data(), value.size());
    response->length = value.size();

    SendBuffer *buf = new SendBuffer;
    buf->chunk = response;
    buf->len = response->length;
    buf->pos = 0;

    m_parent->addData(buf);

    if ((ret = m_parent->writeData()) != ERROR_SUCCESS) {
        ret = ERROR_TCP_SOCKET_WRITE_FAILED;
        return ret;
    }

    return ret;
}

