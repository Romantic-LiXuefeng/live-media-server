#include "lms_http_server_conn.hpp"
#include "kernel_log.hpp"
#include "lms_source.hpp"
#include "kernel_errno.hpp"
#include "lms_http_utility.hpp"
#include "DHttpHeader.hpp"
#include "lms_global.hpp"
#include "DDateTime.hpp"
#include "DMd5.hpp"

lms_http_server_conn::lms_http_server_conn(DThread *parent, DEvent *ev, int fd)
    : lms_conn_base(parent, ev, fd)
    , m_type(HttpType::Default)
    , m_process(NULL)
    , m_code(200)
{
    dint64 timeout = 10 * 1000 * 1000;
    setWriteTimeOut(timeout);
    setReadTimeOut(timeout);

    m_reader = new http_reader(this, true, HTTP_HEADER_CALLBACK(&lms_http_server_conn::onHttpParser));

    m_begin_time = DDateTime::currentDate().toString("yyyy-MM-dd hh:mm:ss.ms");
    m_client_ip = get_peer_ip(fd);
    m_md5 = DMd5::md5(m_client_ip + m_begin_time + DString::number(fd));
}

lms_http_server_conn::~lms_http_server_conn()
{
    DFree(m_reader);
    DFree(m_process);
}

http_reader *lms_http_server_conn::reader()
{
    return m_reader;
}

int lms_http_server_conn::onReadProcess()
{
    int ret = ERROR_SUCCESS;

    global_context->update_id(m_fd);

    if ((ret = m_reader->service()) != ERROR_SUCCESS) {
        if (ret != SOCKET_EAGAIN) {
            log_error("read and parse http request failed. ret=%d", ret);
            return ret;
        }
    }

    switch (m_type) {
    case HttpType::FlvRecv:
    case HttpType::TsRecv:
        ret = m_process->service();
        break;
    case HttpType::SendFile:
    case HttpType::FlvLive:
    case HttpType::TsLive:
        clear_http_body();
        break;
    case HttpType::Default:
        break;
    default:
        break;
    }

    return ret;
}

int lms_http_server_conn::onWriteProcess()
{
    int ret = ERROR_SUCCESS;

    global_context->update_id(m_fd);

    switch (m_type) {
    case HttpType::FlvLive:
    case HttpType::TsLive:
        ret = m_process->flush();
        break;
    case HttpType::SendFile:
        if ((ret = m_process->flush()) != ERROR_SUCCESS) {
            return ret;
        }
        if (m_process->eof()) {
            release();
        }
        break;
    case HttpType::TsRecv:
    case HttpType::FlvRecv:
        break;
    default:
        break;
    }

    return ret;
}

void lms_http_server_conn::onReadTimeOutProcess()
{
    log_error("read timeout");
    release();
}

void lms_http_server_conn::onWriteTimeOutProcess()
{
    log_error("write timeout");
    release();
}

void lms_http_server_conn::onErrorProcess()
{
    log_error("socket error");
    release();
}

void lms_http_server_conn::onCloseProcess()
{
    log_error("socket closed");
    release();
}

int lms_http_server_conn::Process(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    switch (m_type) {
    case HttpType::FlvLive:
    case HttpType::TsLive:
        ret = m_process->process(msg);
        break;
    case HttpType::FlvRecv:
    case HttpType::SendFile:
    case HttpType::TsRecv:
        break;
    default:
        break;
    }

    return ret;
}

void lms_http_server_conn::reload()
{
    if (!m_process->reload()) {
        release();
    }
}

void lms_http_server_conn::release()
{
    if (m_code == 200) {
        http_access_log_end(m_process->request(), m_client_ip, m_md5);
    }

    m_process->release();

    global_context->delete_id(m_fd);

    close();
}

int lms_http_server_conn::onHttpParser(DHttpParser *parser)
{
    int ret = ERROR_SUCCESS;

    DString method = parser->method();
    DString url = parser->getUrl();
    DString uri = url;
    DString param;

    size_t pos = std::string::npos;
    if ((pos = url.find("?")) != std::string::npos) {
        uri = url.substr(0, pos);
        param = url.substr(pos + 1);
    }

    if (method == "GET") {
        DString val = getParam(param, "type");
        if (val == "live") {
            if (uri.endWith(".flv")) {
                m_type = HttpType::FlvLive;
            } else if (uri.endWith(".ts")) {
                m_type = HttpType::TsLive;
            } else {
                m_code = 403;
                ret = ERROR_HTTP_REQUEST_UNSUPPORTED;
                log_error("get uri(%s) is not supported. ret=%d", uri.c_str(), ret);
            }
        } else {
            m_type = HttpType::SendFile;
        }
    } else if (method == "POST") {
        if (uri.endWith(".flv")) {
            m_type = HttpType::FlvRecv;
        } else if (uri.endWith(".ts")) {
            m_type = HttpType::TsRecv;
        } else {
            m_code = 403;
            ret = ERROR_HTTP_REQUEST_UNSUPPORTED;
            log_error("post uri(%s) is not supported. ret=%d", uri.c_str(), ret);
        }
    } else {
        m_code = 403;
        ret = ERROR_HTTP_INVALID_METHOD;
        log_error("method(%s) is not supported. ret=%d", method.c_str(), ret);
    }

    if (m_code != 200) {
        response_http_header(m_code);

        // write access log to file
        http_access_log_begin(parser, m_code, m_begin_time, m_client_ip, m_md5, true);

        return ret;
    }

    return do_process(parser);
}

DString lms_http_server_conn::getParam(const DString &param, const DString &key)
{
    DString value;
    DString _param = param;
    DStringList args = _param.split("&");

    for (int i = 0; i < (int)args.size(); ++i) {
        DStringList temp = args.at(i).split("=");
        if (temp.size() == 2) {
            if (temp.at(0) == key) {
                value = temp.at(1);
            }
        }
    }

    return value;
}

int lms_http_server_conn::do_process(DHttpParser *parser)
{
    int ret = ERROR_SUCCESS;

    switch (m_type) {
    case HttpType::FlvLive:
        m_process = new lms_http_flv_live(this);
        break;
    case HttpType::FlvRecv:
        m_process = new lms_http_flv_recv(this);
        break;
    case HttpType::SendFile:
        m_process = new lms_http_send_file(this);
        break;
    case HttpType::TsLive:
        m_process = new lms_http_ts_live(this);
        break;
    case HttpType::TsRecv:
        m_process = new lms_http_ts_recv(this);
        break;
    default:
        break;
    }

    ret = m_process->initialize(parser);

    if (ret != ERROR_SUCCESS) {
        m_code = 404;

        switch (ret) {
        case ERROR_HTTP_FLV_LIVE_REJECT:
        case ERROR_HTTP_FLV_RECV_REJECT:
        case ERROR_HTTP_SEND_FILE_REJECT:
        case ERROR_HTTP_TS_LIVE_REJECT:
        case ERROR_HTTP_TS_RECV_REJECT:
            m_code = 403;
            break;
        default:         
            break;
        }

        response_http_header(m_code);

        // write access log to file
        http_access_log_begin(parser, m_code, m_begin_time, m_client_ip, m_md5, true);

        return ret;
    }

    http_access_log_begin(parser, 200, m_begin_time, m_client_ip, m_md5, false);

    return m_process->start();
}

void lms_http_server_conn::response_http_header(int code)
{
    DHttpHeader header;
    header.setServer(LMS_VERSION);
    header.setConnectionClose();

    DString str = header.getResponseString(code);

    MemoryChunk *chunk = DMemPool::instance()->getMemory(str.size());
    DSharedPtr<MemoryChunk> response = DSharedPtr<MemoryChunk>(chunk);

    memcpy(response->data, str.data(), str.size());
    response->length = str.size();

    write(response, str.size());
}

void lms_http_server_conn::clear_http_body()
{
    int len = m_reader->getLength();
    if (len > 0) {
        MemoryChunk *chunk = DMemPool::instance()->getMemory(len);
        DSharedPtr<MemoryChunk> payload = DSharedPtr<MemoryChunk>(chunk);

        m_reader->readBody(payload->data, len);
    }
}
