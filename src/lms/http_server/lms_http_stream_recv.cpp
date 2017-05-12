#include "lms_http_stream_recv.hpp"
#include "lms_config.hpp"
#include "DHttpHeader.hpp"
#include "DMemPool.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"
#include "lms_http_utility.hpp"
#include "lms_verify_refer.hpp"

lms_http_stream_recv::lms_http_stream_recv(lms_conn_base *parent)
    : lms_server_stream_base(parent)
    , m_flv(NULL)
{

}

lms_http_stream_recv::~lms_http_stream_recv()
{
    DFree(m_flv);
}

int lms_http_stream_recv::initialize(DHttpParser *parser)
{
    int ret = ERROR_SUCCESS;

    if (!generate_http_request(parser, m_req)) {
        ret = ERROR_GENERATE_REQUEST;
        log_error("http stream recv generate request failed. ret=%d", ret);
        return ret;
    }

    if (!get_config_value(m_req)) {
        ret = ERROR_GET_CONFIG_VALUE;
        log_error("http stream recv get config value failed. ret=%d", ret);
        return ret;
    }

    // 验证refer防盗链
    lms_verify_refer refer;
    if (!refer.check(m_req, true)) {
        ret = ERROR_REFERER_VERIFY;
        log_trace("http stream recv referer verify refused.");
        return ret;
    }

    m_type = get_http_stream_type(parser);

    m_source = lms_source_manager::instance()->addSource(m_req);
    m_source->add_reload(dynamic_cast<lms_conn_base*>(m_parent));

    if (!m_source->onPublish(m_parent->getEvent())) {
        ret = ERROR_SOURCE_ONPUBLISH;
        log_error("http stream recv source onPublish failed. ret=%d", ret);
        return ret;
    }

    m_encode = parser->feild("Transfer-Encoding");

    return ret;
}

int lms_http_stream_recv::start()
{
    int ret = ERROR_SUCCESS;

    m_parent->setRecvTimeOut(m_timeout);
    m_parent->setSendTimeOut(-1);

    if ((ret = response_http_header()) != ERROR_SUCCESS) {
        log_error("http stream recv response http header failed. ret=%d", ret);
        return ret;
    }

    if (m_type == HttpStreamFlv) {
        m_flv = new http_flv_reader(dynamic_cast<DTcpSocket*>(m_parent));
        m_flv->set_av_handler(AV_Handler_CALLBACK(&lms_http_stream_recv::onMessage));

        if (m_encode == "chunked") {
            m_flv->set_chunked(true);
        } else {
            m_flv->set_chunked(false);
        }
    }

    return ret;
}

void lms_http_stream_recv::release()
{
    if (m_source) {
        m_source->onUnpublish();
        m_source->del_reload(dynamic_cast<lms_conn_base*>(m_parent));
    }

    m_source = NULL;
}

int lms_http_stream_recv::do_process()
{
    int ret = ERROR_SUCCESS;

    if (m_flv) {
        if ((ret = m_flv->process()) != ERROR_SUCCESS) {
            log_error("http stream recv flv process failed. ret=%d", ret);
            return ret;
        }

        if (m_flv->eof()) {
            log_trace("http stream recv flv read eof");
            m_parent->release();
        }
    }

    return ERROR_SUCCESS;
}

void lms_http_stream_recv::reload_self(lms_server_config_struct *config)
{
    m_parent->setRecvTimeOut(m_timeout);

    bool enable = config->get_http_enable(m_req);
    if (!enable) {
        m_parent->release();
    }
}

bool lms_http_stream_recv::get_self_config(lms_server_config_struct *config)
{
    return config->get_http_enable(m_req);
}

int lms_http_stream_recv::response_http_header()
{
    int ret = ERROR_SUCCESS;

    DHttpHeader header;
    header.setServer("lms/1.0.0");

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
