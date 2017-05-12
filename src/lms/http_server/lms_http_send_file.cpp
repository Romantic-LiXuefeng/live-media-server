#include "lms_http_send_file.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"
#include "lms_http_utility.hpp"
#include "lms_config.hpp"
#include "DFile.hpp"
#include "lms_verify_refer.hpp"

lms_http_send_file::lms_http_send_file(lms_conn_base *parent)
    : m_parent(parent)
    , m_sendfile(NULL)
    , m_timeout(30 * 1000 * 1000)
{
    m_req = new rtmp_request();
}

lms_http_send_file::~lms_http_send_file()
{
    log_warn("-------------> free lms_http_send_file");

    DFree(m_sendfile);
    DFree(m_req);
}

int lms_http_send_file::initialize(DHttpParser *parser)
{
    int ret = ERROR_SUCCESS;

    if (!generate_http_request(parser, m_req, false)) {
        ret = ERROR_GENERATE_REQUEST;
        log_error("http send file generate http request failed. ret=%d", ret);
        return ret;
    }

    if (!get_config_value(m_req)) {
        ret = ERROR_GET_CONFIG_VALUE;
        log_error("http send file get config value failed. ret=%d", ret);
        return ret;
    }

    // 验证refer防盗链，发送文件属于play
    lms_verify_refer refer;
    if (!refer.check(m_req, false)) {
        ret = ERROR_REFERER_VERIFY;
        log_trace("http send file referer verify refused.");
        return ret;
    }

    DString url = parser->getUrl();
    size_t pos = std::string::npos;
    if ((pos = url.find("?")) != std::string::npos) {
        url = url.substr(0, pos);
    }
    m_filePath = m_root + url;

    log_trace("http send file. filePath=%s", m_filePath.c_str());

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

    m_parent->setSendTimeOut(m_timeout);
    m_parent->setRecvTimeOut(-1);

    m_sendfile = new DSendFile(m_parent->GetDescriptor());

    if (m_sendfile->open(m_filePath) != ERROR_SUCCESS) {
        ret = ERROR_OPEN_FILE;
        log_error("http send file open failed. filePath=%s, ret=%d", m_filePath.c_str(), ret);
        return ret;
    }

    if ((ret = response_header(m_sendfile->getFileSize())) != ERROR_SUCCESS) {
        log_error("http send file response http header failed. ret=%d", ret);
        return ret;
    }

    if ((ret = sendFile()) != ERROR_SUCCESS) {
        log_error("http send file start send failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int lms_http_send_file::sendFile()
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

        if (m_sendfile->eof()) {
            log_trace("http send file completed");
            release();
        }
    }

    return ret;
}

void lms_http_send_file::release()
{
    m_parent->release();
}

int lms_http_send_file::response_header(int size)
{
    int ret = ERROR_SUCCESS;

    DHttpHeader header;
    header.setServer("lms/1.0.0");
    header.setContentLength(size);

    DString type;

    size_t pos = std::string::npos;
    if ((pos = m_req->stream.find(".")) != std::string::npos) {
        type = m_req->stream.substr(pos + 1);
    }

    type = header.getContentType(type);
    header.setContentType(type);

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

bool lms_http_send_file::get_config_value(rtmp_request *req)
{
    lms_server_config_struct *config = lms_config::instance()->get_server(req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        return false;
    }

    int timeout = config->get_timeout(req);
    m_timeout = timeout * 1000 * 1000;

    m_root = config->get_http_root(req);

    if (!config->get_http_enable(req)) {
        return false;
    }

    return true;
}
