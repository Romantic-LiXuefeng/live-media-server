#include "lms_http_send_file.hpp"
#include "lms_http_server_conn.hpp"
#include "kernel_log.hpp"
#include "kernel_errno.hpp"
#include "lms_config.hpp"
#include "lms_http_utility.hpp"
#include "DFile.hpp"
#include "lms_global.hpp"
#include "DHttpHeader.hpp"

lms_http_send_file::lms_http_send_file(lms_http_server_conn *conn)
    : m_conn(conn)
    , m_req(NULL)
    , m_sendfile(NULL)
    , m_enable(false)
    , m_timeout(10 * 1000 * 1000)
{

}

lms_http_send_file::~lms_http_send_file()
{
    DFree(m_req);
    DFree(m_sendfile);
}

int lms_http_send_file::initialize(DHttpParser *parser)
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
        ret = ERROR_HTTP_SEND_FILE_REJECT;
        return ret;
    }

    DString url = parser->getUrl();
    size_t pos = std::string::npos;
    if ((pos = url.find("?")) != std::string::npos) {
        url = url.substr(0, pos);
    }
    m_filePath = m_root + url;

    if (!DFile::exists(m_filePath)) {
        ret = ERROR_FILE_NOT_EXIST;
        log_error("file is not exist. filePath=%s, ret=%d", m_filePath.c_str(), ret);
        return ret;
    }

    return ret;
}

int lms_http_send_file::start()
{
    int ret = ERROR_SUCCESS;

    m_conn->setReadTimeOut(-1);
    m_conn->setWriteTimeOut(m_timeout);

    m_sendfile = new DSendFile(m_conn->GetDescriptor());

    if (m_sendfile->open(m_filePath) != ERROR_SUCCESS) {
        ret = ERROR_OPEN_FILE;
        log_error("http send file open failed. filePath=%s, ret=%d", m_filePath.c_str(), ret);
        return ret;
    }

    if ((ret = response_http_header()) != ERROR_SUCCESS) {
        if (ret != SOCKET_EAGAIN) {
            return ret;
        }
    }

    return ret;
}

int lms_http_send_file::flush()
{
    int ret = ERROR_SUCCESS;

    if (m_sendfile) {
        // 发送小文件时，start时直接就发完了，但是write回调依然会执行，防止出现重复日志，在这加一个判断
        if (m_sendfile->eof()) {
            return ret;
        }

        if (m_sendfile->sendFile() != ERROR_SUCCESS) {
            ret = ERROR_SEND_FILE;
            log_error("http send file send process failed. ret=%d", ret);
            return ret;
        }
    }

    return ret;
}

bool lms_http_send_file::eof()
{
    return m_sendfile->eof();
}

bool lms_http_send_file::reload()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        return false;
    }

    int timeout = config->get_http_timeout(m_req);
    m_timeout = timeout * 1000 * 1000;

    if (!config->get_http_enable(m_req)) {
        return false;
    }

    return true;
}

void lms_http_send_file::release()
{
    // nothing to do
}

void lms_http_send_file::get_config_value()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        return;
    }

    int timeout = config->get_http_timeout(m_req);
    m_timeout = timeout * 1000 * 1000;

    m_root = config->get_http_root(m_req);

    if (config->get_http_enable(m_req)) {
        m_enable = true;
    }
}

int lms_http_send_file::response_http_header()
{
    int ret = ERROR_SUCCESS;

    DHttpHeader header;
    header.setServer(LMS_VERSION);

    dint64 size = m_sendfile->getFileSize();
    header.setContentLength(size);

    size_t pos = std::string::npos;
    if ((pos = m_req->stream.find(".")) != std::string::npos) {
        DString type = m_req->stream.substr(pos + 1);
        header.setContentType(type);
    }

    header.setConnectionClose();

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
