#include "lms_http_server_conn.hpp"
#include "kernel_log.hpp"
#include "lms_source.hpp"
#include "kernel_errno.hpp"
#include "lms_http_utility.hpp"
#include "DHttpHeader.hpp"
#include "lms_global.hpp"

lms_http_server_conn::lms_http_server_conn(DThread *parent, DEvent *ev, int fd)
    : lms_conn_base(parent, ev, fd)
    , m_type(HttpType::Default)
    , m_flv_live(NULL)
    , m_flv_recv(NULL)
    , m_send_file(NULL)
    , m_ts_live(NULL)
    , m_ts_recv(NULL)
{
    dint64 timeout = 10 * 1000 * 1000;
    setWriteTimeOut(timeout);
    setReadTimeOut(timeout);

    m_reader = new http_reader(this, true, HTTP_HEADER_CALLBACK(&lms_http_server_conn::onHttpParser));
}

lms_http_server_conn::~lms_http_server_conn()
{
    DFree(m_reader);
    DFree(m_flv_live);
    DFree(m_flv_recv);
    DFree(m_send_file);
    DFree(m_ts_live);
    DFree(m_ts_recv);
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
        if (m_flv_recv) {
            ret = m_flv_recv->service();
        }
        break;
    case HttpType::TsRecv:
        if (m_ts_recv) {
            ret = m_ts_recv->service();
        }
        break;
    case HttpType::SendFile:
        clear_http_body();
        break;
    case HttpType::FlvLive:
        clear_http_body();
        break;
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
        ret = m_flv_live->flush();
        break;
    case HttpType::FlvRecv:

        break;
    case HttpType::SendFile:
        if ((ret = m_send_file->flush()) != ERROR_SUCCESS) {
            return ret;
        }
        if (m_send_file->eof()) {
            release();
        }
        break;
    case HttpType::TsLive:
        ret = m_ts_live->flush();
        break;
    case HttpType::TsRecv:

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
        ret = m_flv_live->process(msg);
        break;
    case HttpType::FlvRecv:

        break;
    case HttpType::SendFile:

        break;
    case HttpType::TsLive:
        ret = m_ts_live->process(msg);
        break;
    case HttpType::TsRecv:

        break;
    default:
        break;
    }

    return ret;
}

void lms_http_server_conn::reload()
{
    bool ret;

    switch (m_type) {
    case HttpType::FlvLive:
        ret = m_flv_live->reload();
        break;
    case HttpType::FlvRecv:
        ret = m_flv_recv->reload();
        break;
    case HttpType::SendFile:
        ret = m_send_file->reload();
        break;
    case HttpType::TsLive:
        ret = m_ts_live->reload();
        break;
    case HttpType::TsRecv:
        ret = m_ts_recv->reload();
        break;
    default:
        break;
    }

    if (!ret) {
        release();
    }
}

void lms_http_server_conn::release()
{
    switch (m_type) {
    case HttpType::FlvLive:
        m_flv_live->release();
        break;
    case HttpType::FlvRecv:
        m_flv_recv->release();
        break;
    case HttpType::SendFile:
        m_send_file->release();
        break;
    case HttpType::TsLive:
        m_ts_live->release();
        break;
    case HttpType::TsRecv:
        m_ts_recv->release();
        break;
    default:
        break;
    }

    global_context->delete_id(m_fd);

    close();
}

int lms_http_server_conn::onHttpParser(DHttpParser *parser)
{
    int ret = ERROR_SUCCESS;

    int code = 200;

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
                code = 403;
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
            code = 403;
            ret = ERROR_HTTP_REQUEST_UNSUPPORTED;
            log_error("post uri(%s) is not supported. ret=%d", uri.c_str(), ret);
        }
    } else {
        code = 403;
        ret = ERROR_HTTP_INVALID_METHOD;
        log_error("method(%s) is not supported. ret=%d", method.c_str(), ret);
    }

    if (code != 200) {
        response_http_header(code);
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
        ret = init_flv_live(parser);
        break;
    case HttpType::FlvRecv:
        ret = init_flv_recv(parser);
        break;
    case HttpType::SendFile:
        ret = init_send_file(parser);
        break;
    case HttpType::TsLive:
        ret = init_ts_live(parser);
        break;
    case HttpType::TsRecv:
        ret = init_ts_recv(parser);
        break;
    default:
        break;
    }

    if (ret != ERROR_SUCCESS) {
        int code = 200;

        switch (m_type) {
        case HttpType::FlvRecv:
        case HttpType::TsRecv:
            code = 403;
            break;
        default:
            code = 404;
            break;
        }

        response_http_header(code);

        return ret;
    }

    return start_process();
}

int lms_http_server_conn::init_flv_live(DHttpParser *parser)
{
    m_flv_live = new lms_http_flv_live(this);

    return m_flv_live->initialize(parser);
}

int lms_http_server_conn::init_flv_recv(DHttpParser *parser)
{
    m_flv_recv = new lms_http_flv_recv(this);

    return m_flv_recv->initialize(parser);
}

int lms_http_server_conn::init_send_file(DHttpParser *parser)
{
    m_send_file = new lms_http_send_file(this);

    return m_send_file->initialize(parser);
}

int lms_http_server_conn::init_ts_live(DHttpParser *parser)
{
    m_ts_live = new lms_http_ts_live(this);

    return m_ts_live->initialize(parser);
}

int lms_http_server_conn::init_ts_recv(DHttpParser *parser)
{
    m_ts_recv = new lms_http_ts_recv(this);

    return m_ts_recv->initialize(parser);
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

int lms_http_server_conn::start_process()
{
    int ret = ERROR_SUCCESS;

    switch (m_type) {
    case HttpType::FlvLive:
        ret = start_flv_live();
        break;
    case HttpType::FlvRecv:
        ret = start_flv_recv();
        break;
    case HttpType::SendFile:
        ret = start_send_file();
        break;
    case HttpType::TsLive:
        ret = start_ts_live();
        break;
    case HttpType::TsRecv:
        ret = start_ts_recv();
        break;
    default:
        break;
    }

    return ret;
}

int lms_http_server_conn::start_flv_live()
{
    return m_flv_live->start();
}

int lms_http_server_conn::start_flv_recv()
{
    return m_flv_recv->start();
}

int lms_http_server_conn::start_send_file()
{
    return m_send_file->start();
}

int lms_http_server_conn::start_ts_live()
{
    return m_ts_live->start();
}

int lms_http_server_conn::start_ts_recv()
{
    return m_ts_recv->start();
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
