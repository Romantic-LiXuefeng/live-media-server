#include "lms_http_client_flv_play.hpp"
#include "kernel_errno.hpp"
#include "lms_source.hpp"
#include "kernel_log.hpp"
#include "lms_config.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

lms_http_client_flv_play::lms_http_client_flv_play(DEvent *event, lms_source *source)
    : lms_client_stream_base(event, source)
    , m_http_header(false)
{
    m_flv = new http_flv_reader(this);
    m_flv->set_av_handler(AV_Handler_CALLBACK(&lms_http_client_flv_play::onMessage));

    m_parser = new DHttpParser(HTTP_RESPONSE);

    setRecvTimeOut(m_timeout);
    setSendTimeOut(m_timeout);
}

lms_http_client_flv_play::~lms_http_client_flv_play()
{
    log_warn("free --> lms_http_client_flv_play");
    DFree(m_flv);
    DFree(m_parser);
}

int lms_http_client_flv_play::start_handler()
{
    int ret = ERROR_SUCCESS;

    if ((ret = send_http_header()) != ERROR_SUCCESS) {
        log_error("http flv play client send http header failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int lms_http_client_flv_play::do_process()
{
    int ret = ERROR_SUCCESS;

    if ((ret = read_header()) != ERROR_SUCCESS) {
        log_error("http flv play client read http header failed. ret=%d", ret);
        return ret;
    }

    if (m_flv) {
        if ((ret = m_flv->process()) != ERROR_SUCCESS) {
            log_error("http flv play client reader process failed. ret=%d", ret);
            return ret;
        }

        if (m_flv->eof()) {
            log_trace("http flv play client read eof");
            return ERROR_HTTP_READ_EOF;
        }
    }

    return ret;
}

int lms_http_client_flv_play::send_http_header()
{
    int ret = ERROR_SUCCESS;

    DHttpHeader header;
    header.setConnectionKeepAlive();
    header.addValue("Accept", "*/*");
    header.setHost(m_req->vhost);

    DString value = header.getRequestString("GET", "/" + m_req->app + "/" + m_req->stream + ".flv");

    MemoryChunk *chunk = DMemPool::instance()->getMemory(value.size());
    DSharedPtr<MemoryChunk> response = DSharedPtr<MemoryChunk>(chunk);

    memcpy(response->data, value.data(), value.size());
    response->length = value.size();

    SendBuffer *buf = new SendBuffer;
    buf->chunk = response;
    buf->len = response->length;
    buf->pos = 0;

    addData(buf);

    if ((ret = writeData()) != ERROR_SUCCESS) {
        ret = ERROR_TCP_SOCKET_WRITE_FAILED;
        return ret;
    }

    return ret;
}

int lms_http_client_flv_play::read_header()
{
    int ret = ERROR_SUCCESS;

    if (m_http_header) {
        return ret;
    }

    int len = getBufferLen();

    MemoryChunk *buf = DMemPool::instance()->getMemory(len);
    DAutoFree(MemoryChunk, buf);

    if (copyData(buf->data, len) < 0) {
        ret = ERROR_TCP_SOCKET_COPY_FAILED;
        log_error("http flv play client copy http header data filed. ret=%d", ret);
        return ret;
    }

    buf->length = len;

    if (m_parser->parse(buf->data, buf->length) < 0) {
        ret = ERROR_HTTP_HEADER_PARSER;
        log_error("http flv play client parse http header failed. ret=%d", ret);
        return ret;
    }

    if (m_parser->completed()) {
        DString content(buf->data, buf->length);

        DString delim("\r\n\r\n");
        size_t index = content.find(delim);

        if (index != DString::npos) {
            DString body = content.substr(0, index + delim.size());

            MemoryChunk *chunk = DMemPool::instance()->getMemory(body.size());
            DAutoFree(MemoryChunk, chunk);

            if (readData(chunk->data, body.size()) < 0) {
                ret = ERROR_TCP_SOCKET_READ_FAILED;
                log_error("http flv play client read http header for reduce data failed. ret=%d", ret);
                return ret;
            }
        }

        duint16 status_code = m_parser->statusCode();

        if (status_code != 200) {
            release();
        } else {
            setSendTimeOut(-1);
            setRecvTimeOut(m_timeout);

            DString encoding = m_parser->feild("Transfer-Encoding");
            if (encoding == "chunked") {
                m_flv->set_chunked(true);
            } else {
                m_flv->set_chunked(false);
            }
        }

        m_http_header = true;
    }

    return ret;
}
