#include "lms_http_server_conn.hpp"
#include "lms_source.hpp"
#include "DGlobal.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"
#include "lms_http_utility.hpp"

namespace HttpSchedule {
    enum Schedule { ReadHeader = 0, StreamLive, StreamRecv, SendFile, Refuse };
}

lms_http_server_conn::lms_http_server_conn(DThread *parent, DEvent *ev, int fd)
    : lms_conn_base(parent, ev, fd)
    , m_schedule(HttpSchedule::ReadHeader)
    , m_stream_live(NULL)
    , m_stream_recv(NULL)
    , m_sendfile(NULL)
{
    m_parser = new DHttpParser(HTTP_REQUEST);

    setSendTimeOut(30 * 1000 * 1000);
    setRecvTimeOut(30 * 1000 * 1000);
}

lms_http_server_conn::~lms_http_server_conn()
{
    log_warn("-------------> free lms_http_server_conn");

    DFree(m_stream_live);
    DFree(m_stream_recv);
    DFree(m_parser);
    DFree(m_sendfile);
}

int lms_http_server_conn::onReadProcess()
{
    int ret = ERROR_SUCCESS;

    global_context->update_id(m_fd);

    if (m_schedule == HttpSchedule::ReadHeader) {
        if ((ret = read_header()) != ERROR_SUCCESS) {
            log_error("read and process http header failed. ret=%d", ret);
            return ret;
        }
    }

    if (m_schedule == HttpSchedule::StreamLive || m_schedule == HttpSchedule::SendFile) {
        int len = getBufferLen();
        if (len > 0) {
            MemoryChunk *chunk = DMemPool::instance()->getMemory(len);
            DAutoFree(MemoryChunk, chunk);

            if (readData(chunk->data, len) < 0) {
                ret = ERROR_TCP_SOCKET_READ_FAILED;
                log_error("http server conn read useless data failed. ret=%d", ret);
                return ret;
            }
        }
    }

    if (m_schedule == HttpSchedule::StreamRecv) {
        if (m_stream_recv) {
            if ((ret = m_stream_recv->do_process()) != ERROR_SUCCESS) {
                log_error("http_stream_recv process failed. ret=%d", ret);
                return ret;
            }
        }
    }

    return ret;
}

int lms_http_server_conn::onWriteProcess()
{
    int ret = ERROR_SUCCESS;

    global_context->update_id(m_fd);

    if (m_schedule == HttpSchedule::SendFile) {
        if (m_sendfile) {
            if ((ret = m_sendfile->sendFile()) != ERROR_SUCCESS) {
                log_error("http server conn send file to client failed. ret=%d", ret);
                return ret;
            }
        }
        return ret;
    }

    if ((ret = writeData()) != ERROR_SUCCESS) {
        ret = ERROR_TCP_SOCKET_WRITE_FAILED;
        log_error("http_server_conn write data failed. ret=%d", ret);
        return ret;
    }

    if (m_schedule == HttpSchedule::Refuse) {
        release();
    }

    return ret;
}

void lms_http_server_conn::onReadTimeOutProcess()
{
    release();
}

void lms_http_server_conn::onWriteTimeOutProcess()
{
    release();
}

void lms_http_server_conn::onErrorProcess()
{
    release();
}

void lms_http_server_conn::onCloseProcess()
{
    release();
}

int lms_http_server_conn::Process(CommonMessage *_msg)
{
    if (m_stream_live) {
        return m_stream_live->process(_msg);
    }
    return ERROR_SUCCESS;
}

void lms_http_server_conn::reload()
{
    if (m_schedule == HttpSchedule::StreamLive) {
        if (m_stream_live) {
            m_stream_live->reload();
        }
    }

    if (m_schedule == HttpSchedule::StreamRecv) {
        if (m_stream_recv) {
            m_stream_recv->reload();
        }
    }
}

int lms_http_server_conn::read_header()
{
    int ret = ERROR_SUCCESS;

    int len = getBufferLen();

    MemoryChunk *buf = DMemPool::instance()->getMemory(len);
    DAutoFree(MemoryChunk, buf);

    if (copyData(buf->data, len) < 0) {
        ret = ERROR_TCP_SOCKET_COPY_FAILED;
        log_error("http server conn copy http header data filed. ret=%d", ret);
        return ret;
    }

    buf->length = len;

    if (m_parser->parse(buf->data, buf->length) < 0) {
        ret = ERROR_HTTP_HEADER_PARSER;
        log_error("http server conn parse http header failed. ret=%d", ret);
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
                log_error("http server conn read http header for reduce data failed. ret=%d", ret);
                return ret;
            }
        }

        return parse_header();
    }

    return ret;
}

int lms_http_server_conn::parse_header()
{
    int ret = ERROR_SUCCESS;

    DString _method = m_parser->method();
    DString _url = m_parser->getUrl();

    log_info("http request. type=%s, url=%s", _method.c_str(), _url.c_str());

    if (_method == "GET") {
        ret = process_get(_url);
    } else if (_method == "POST") {
        ret = process_post(_url);
    }

    return ret;
}

int lms_http_server_conn::process_get(const DString &_url)
{
    int ret = ERROR_SUCCESS;

    size_t pos = std::string::npos;
    DString url = _url;
    DString type;

    if ((pos = _url.find("?")) != std::string::npos) {
        url = _url.substr(0, pos);
    }

    log_trace("http get. url=%s", url.c_str());

    if (url.endWith(".flv") || url.endWith(".ts")) {
        DString param = _url.substr(pos + 1);
        DStringList args = param.split("&");
        for (int i = 0; i < (int)args.size(); ++i) {
            DStringList temp = args.at(i).split("=");
            if (temp.size() == 2) {
                if (temp.at(0) == "type") {
                    type = temp.at(1);
                }
            }
        }

        if (type == "live") {
            return start_stream_live();
        } else {
            return start_send_file();
        }
    } else {
        return start_send_file();
    }

    return ret;
}

int lms_http_server_conn::process_post(const DString &_url)
{
    int ret = ERROR_SUCCESS;

    size_t pos = std::string::npos;
    DString url = _url;

    if ((pos = _url.find("?")) != std::string::npos) {
        url = _url.substr(0, pos);
    }

    log_trace("http post. url=%s", url.c_str());

    if (url.endWith(".flv") || url.endWith(".ts")) {
        return start_stream_recv();
    } else {
        log_trace("refuse http post with 403. url=%s", url.c_str());
        return response_http_refuse(403, "Forbidden");
    }

    return ret;
}

int lms_http_server_conn::start_send_file()
{
    int ret = ERROR_SUCCESS;

    m_schedule = HttpSchedule::SendFile;

    m_sendfile = new lms_http_send_file(this);

    if ((ret = m_sendfile->initialize(m_parser)) != ERROR_SUCCESS) {
        log_error("send file initialize failed. ret=%d", ret);
        response_http_refuse(404, "Not Found");
        return ret;
    }

    if ((ret = m_sendfile->start()) != ERROR_SUCCESS) {
        log_error("send file start failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int lms_http_server_conn::start_stream_live()
{
    int ret = ERROR_SUCCESS;

    m_schedule = HttpSchedule::StreamLive;

    m_stream_live = new lms_http_stream_live(this);

    if ((ret = m_stream_live->initialize(m_parser)) != ERROR_SUCCESS) {
        log_error("http stream live initialize failed. ret=%d", ret);
        response_http_refuse(404, "Not Found");
        return ret;
    }

    if ((ret = m_stream_live->start()) != ERROR_SUCCESS) {
        log_error("stream live start failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int lms_http_server_conn::start_stream_recv()
{
    int ret = ERROR_SUCCESS;

    m_schedule = HttpSchedule::StreamRecv;

    m_stream_recv = new lms_http_stream_recv(this);

    if ((ret = m_stream_recv->initialize(m_parser)) != ERROR_SUCCESS) {
        log_error("http stream recv initialize failed. ret=%d", ret);
        response_http_refuse(403, "Forbidden");
        return ret;
    }

    if ((ret = m_stream_recv->start()) != ERROR_SUCCESS) {
        log_error("stream recv start failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int lms_http_server_conn::response_http_refuse(int err, const DString &info)
{
    int ret = ERROR_SUCCESS;

    log_trace("refuse http request with %s(%d)", info.c_str(), err);

    m_schedule = HttpSchedule::Refuse;

    DHttpHeader header;
    header.setServer("lms/1.0.0");
    header.setConnectionClose();
    DString str = header.getResponseString(err, info);

    MemoryChunk *chunk = DMemPool::instance()->getMemory(str.size());
    DSharedPtr<MemoryChunk> response = DSharedPtr<MemoryChunk>(chunk);

    memcpy(response->data, str.data(), str.size());
    response->length = str.size();

    SendBuffer *buf = new SendBuffer;
    buf->chunk = response;
    buf->len = response->length;
    buf->pos = 0;

    addData(buf);

    if (writeData() != ERROR_SUCCESS) {
        ret = ERROR_TCP_SOCKET_WRITE_FAILED;
        log_error("response http refuse %s(%d) header failed. ret=%d", info.c_str(), err, ret);
        return ret;
    }

    return ret;
}

void lms_http_server_conn::release()
{
    global_context->update_id(m_fd);
    log_trace("http connection close");

    if (m_schedule == HttpSchedule::StreamLive) {
        if (m_stream_live) {
            m_stream_live->release();
        }
    }

    if (m_schedule == HttpSchedule::StreamRecv) {
        if (m_stream_recv) {
            m_stream_recv->release();
        }
    }

    global_context->delete_id(m_fd);
    closeSocket();
}
