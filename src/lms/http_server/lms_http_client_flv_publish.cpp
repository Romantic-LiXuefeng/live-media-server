#include "lms_http_client_flv_publish.hpp"
#include "kernel_errno.hpp"
#include "DGlobal.hpp"
#include "kernel_log.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

lms_http_client_flv_publish::lms_http_client_flv_publish(DEvent *event, lms_source *source)
    : lms_client_stream_base(event, source)
    , m_chunked(true)
    , m_responsed(false)
    , m_can_publish(false)
{
    m_flv = new http_flv_writer(this);

    m_parser = new DHttpParser(HTTP_RESPONSE);

    setRecvTimeOut(m_timeout);
    setSendTimeOut(m_timeout);
}

lms_http_client_flv_publish::~lms_http_client_flv_publish()
{
    log_warn("free --> lms_http_client_flv_publish");

    DFree(m_flv);
    DFree(m_parser);
}

void lms_http_client_flv_publish::set_chunked(bool chunked)
{
    m_chunked = chunked;
    m_flv->set_chunked(chunked);
}

int lms_http_client_flv_publish::start_handler()
{
    int ret = ERROR_SUCCESS;

    if ((ret = send_http_header()) != ERROR_SUCCESS) {
        log_error("send http header failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int lms_http_client_flv_publish::do_process()
{
    int ret = ERROR_SUCCESS;

    if ((ret = read_header()) != ERROR_SUCCESS) {
        log_error("http flv publish client read http header failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int lms_http_client_flv_publish::send_av_data(CommonMessage *msg)
{
    m_flv->send_av_data(msg);

    return ERROR_SUCCESS;
}

bool lms_http_client_flv_publish::can_publish()
{
    return m_can_publish;
}

int lms_http_client_flv_publish::send_http_header()
{
    int ret = ERROR_SUCCESS;

    DHttpHeader header;
    header.setConnectionClose();
    header.addValue("Accept", "*/*");
    header.setHost(m_req->vhost);
    header.addValue("Icy-MetaData", "1");

    if (m_chunked) {
        header.addValue("Transfer-Encoding", "chunked");
    }

    DString value = header.getRequestString("POST", "/" + m_req->app + "/" + m_req->stream + ".flv");

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
        log_error("http flv publish client send http header failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int lms_http_client_flv_publish::read_header()
{
    int ret = ERROR_SUCCESS;

    if (m_responsed) {
        // 开始发送数据之后，不排除对端继续发数据的可能，直接忽略
        int length = getBufferLen();
        MemoryChunk *chunk = DMemPool::instance()->getMemory(length);
        DAutoFree(MemoryChunk, chunk);

        if (readData(chunk->data, length) < 0) {
            ret = ERROR_TCP_SOCKET_READ_FAILED;
            log_error("http flv publish client read additional data failed");
            return -1;
        }
        return ret;
    }

    int len = getBufferLen();

    MemoryChunk *buf = DMemPool::instance()->getMemory(len);
    DAutoFree(MemoryChunk, buf);

    if (copyData(buf->data, len) < 0) {
        ret = ERROR_TCP_SOCKET_COPY_FAILED;
        log_error("http flv publish client copy http header data filed. ret=%d", ret);
        return ret;
    }

    buf->length = len;

    if (m_parser->parse(buf->data, buf->length) < 0) {
        ret = ERROR_HTTP_HEADER_PARSER;
        log_error("http flv publish client parse http header failed. ret=%d", ret);
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
                log_error("http flv publish client read http header for reduce data failed. ret=%d", ret);
                return ret;
            }
        }

        duint16 status_code = m_parser->statusCode();

        if (status_code != 200) {
            release();
        } else {
            m_can_publish = true;

            setRecvTimeOut(-1);
            setSendTimeOut(m_timeout);

            m_flv->send_flv_header();
        }

        m_responsed = true;
    }

    return ret;
}


