#include "lms_verify_hooks.hpp"
#include "kernel_log.hpp"
#include "writer.h"
#include "stringbuffer.h"
#include "document.h"
#include "kernel_errno.hpp"
#include "DHttpHeader.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#define DEFAULT_HOOK_TIMEOUT    10 * 1000 * 1000

using namespace rapidjson;

namespace HookSchedule {
    enum schedule { Header = 0, Body, Completed, Release };
}

lms_verify_hooks::lms_verify_hooks(DEvent *event)
    : DTcpSocket(event)
    , m_schedule(HookSchedule::Header)
    , m_content_length(0)
    , m_handler(NULL)
    , m_result(false)
    , m_post_started(false)
    , m_timeout(DEFAULT_HOOK_TIMEOUT)
{
    m_parser = new DHttpParser(HTTP_RESPONSE);
    m_reader = new http_reader(this);
}

lms_verify_hooks::~lms_verify_hooks()
{
    log_warn("free --> lms_verify_hooks");

    DFree(m_parser);
    DFree(m_reader);
}

void lms_verify_hooks::set_timeout(dint64 timeout)
{
    m_timeout = timeout;
}

void lms_verify_hooks::set_handler(HookHandlerEvent handler)
{
    m_handler = handler;
}

void lms_verify_hooks::on_connect(rtmp_request *req, duint64 id, const DString &url, const DString &ip)
{
    m_id = id;

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);

    writer.StartObject();

    writer.Key("action");
    writer.String("on_connect");

    writer.Key("ip");
    writer.String(ip.c_str());

    writer.Key("client_id");
    writer.Uint64(id);

    writer.Key("vhost");
    writer.String(req->vhost.c_str());

    writer.Key("app");
    writer.String(req->app.c_str());

    writer.Key("tcUrl");
    writer.String(req->tcUrl.c_str());

    writer.Key("pageUrl");
    writer.String(req->pageUrl.c_str());

    writer.EndObject();

    m_value = buffer.GetString();

    m_uri.initialize(url);

    log_trace("hook verify on_connect: %s", m_value.c_str());

    lookup_host();
}

void lms_verify_hooks::on_publish(rtmp_request *req, duint64 id, const DString &url, const DString &ip)
{
    m_id = id;

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);

    writer.StartObject();

    writer.Key("action");
    writer.String("on_publish");

    writer.Key("ip");
    writer.String(ip.c_str());

    writer.Key("client_id");
    writer.Uint64(id);

    writer.Key("vhost");
    writer.String(req->vhost.c_str());

    writer.Key("app");
    writer.String(req->app.c_str());

    writer.Key("tcUrl");
    writer.String(req->tcUrl.c_str());

    writer.Key("stream");
    writer.String(req->stream.c_str());

    writer.EndObject();

    m_value = buffer.GetString();

    m_uri.initialize(url);

    log_trace("hook verify on_publish: %s", m_value.c_str());

    lookup_host();
}

void lms_verify_hooks::on_play(rtmp_request *req, duint64 id, const DString &url, const DString &ip)
{
    m_id = id;

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);

    writer.StartObject();

    writer.Key("action");
    writer.String("on_play");

    writer.Key("ip");
    writer.String(ip.c_str());

    writer.Key("client_id");
    writer.Uint64(id);

    writer.Key("vhost");
    writer.String(req->vhost.c_str());

    writer.Key("app");
    writer.String(req->app.c_str());

    writer.Key("tcUrl");
    writer.String(req->tcUrl.c_str());

    writer.Key("stream");
    writer.String(req->stream.c_str());

    writer.EndObject();

    m_value = buffer.GetString();

    m_uri.initialize(url);

    log_trace("hook verify on_play: %s", m_value.c_str());

    lookup_host();
}

void lms_verify_hooks::on_unpublish(rtmp_request *req, duint64 id, const DString &url, const DString &ip)
{
    m_id = id;

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);

    writer.StartObject();

    writer.Key("action");
    writer.String("on_unpublish");

    writer.Key("ip");
    writer.String(ip.c_str());

    writer.Key("client_id");
    writer.Uint64(id);

    writer.Key("vhost");
    writer.String(req->vhost.c_str());

    writer.Key("app");
    writer.String(req->app.c_str());

    writer.Key("stream");
    writer.String(req->stream.c_str());

    writer.EndObject();

    m_value = buffer.GetString();

    m_uri.initialize(url);

    log_trace("hook verify on_unpublish: %s", m_value.c_str());

    lookup_host();
}

void lms_verify_hooks::on_stop(rtmp_request *req, duint64 id, const DString &url, const DString &ip)
{
    m_id = id;

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);

    writer.StartObject();

    writer.Key("action");
    writer.String("on_stop");

    writer.Key("ip");
    writer.String(ip.c_str());

    writer.Key("client_id");
    writer.Uint64(id);

    writer.Key("vhost");
    writer.String(req->vhost.c_str());

    writer.Key("app");
    writer.String(req->app.c_str());

    writer.Key("stream");
    writer.String(req->stream.c_str());

    writer.EndObject();

    m_value = buffer.GetString();

    m_uri.initialize(url);

    log_trace("hook verify on_unpublish: %s", m_value.c_str());

    lookup_host();
}

int lms_verify_hooks::onReadProcess()
{
    int ret = ERROR_SUCCESS;

    global_context->update_id(m_fd);

    if ((ret = read_header()) != ERROR_SUCCESS) {
        log_error("hook verify read http header failed. ret=%d", ret);
        return ret;
    }

    if ((ret = read_body()) != ERROR_SUCCESS) {
        log_error("hook verify read http body failed. ret=%d", ret);
        return ret;
    }

    if ((ret = parse_body()) != ERROR_SUCCESS) {
        return ret;
    }

    return ret;
}

int lms_verify_hooks::onWriteProcess()
{
    int ret = ERROR_SUCCESS;

    global_context->update_id(m_fd);

    do_post();

    if ((ret = writeData()) != ERROR_SUCCESS) {
        ret = ERROR_TCP_SOCKET_WRITE_FAILED;
        return ret;
    }

    return ret;
}

void lms_verify_hooks::onReadTimeOutProcess()
{
    release();
}

void lms_verify_hooks::onWriteTimeOutProcess()
{
    release();
}

void lms_verify_hooks::onErrorProcess()
{
    release();
}

void lms_verify_hooks::onCloseProcess()
{
    release();
}

void lms_verify_hooks::lookup_host()
{
    DHostInfo *h = new DHostInfo(m_event);
    h->setFinishedHandler(HOST_CALLBACK(&lms_verify_hooks::onHost));
    h->lookupHost(m_uri.get_host());
}

void lms_verify_hooks::onHost(const DStringList &ips)
{
    global_context->update(m_id);

    if (ips.empty()) {
        log_error("hook verify dns host is empty");
        release();
    }

    DString ip = ips.at(0);
    int port = m_uri.get_port();

    int ret = connectToHost(ip.c_str(), port);

    if (ret == EINPROGRESS) {
        return;
    } else if (ret != ERROR_SUCCESS) {
        ret = ERROR_TCP_SOCKET_CONNECT;
        log_error("hook verify connect to host failed. ip=%s, port=%d, ret=%d", ip.c_str(), port, ret);
        release();
    } else {
        do_post();

        if ((ret = writeData()) != ERROR_SUCCESS) {
            ret = ERROR_TCP_SOCKET_WRITE_FAILED;
            log_error("hook verify write data failed. ret=%d", ret);
            release();
        }
    }
}

void lms_verify_hooks::do_post()
{
    if (m_post_started) {
        return;
    }

    global_context->set_id(m_fd);
    global_context->update_id(m_fd);

    log_trace("hook verify client_id, hook_id: %llu[%llu]", m_id, global_context->get_id());

    setRecvTimeOut(m_timeout);
    setSendTimeOut(m_timeout);

    DHttpHeader header;
    header.setContentLength(m_value.length());
    header.setHost(m_uri.get_host());

    DString value = header.getRequestString("POST", m_uri.get_path());

    value.append(m_value);

    MemoryChunk *chunk = DMemPool::instance()->getMemory(value.size());
    DSharedPtr<MemoryChunk> response = DSharedPtr<MemoryChunk>(chunk);

    memcpy(response->data, value.data(), value.size());
    response->length = value.size();

    SendBuffer *buf = new SendBuffer;
    buf->chunk = response;
    buf->len = response->length;
    buf->pos = 0;

    addData(buf);

    m_post_started = true;
}

void lms_verify_hooks::release()
{
    global_context->update_id(m_fd);

    if (m_schedule == HookSchedule::Release) {
        return;
    }

    verify();

    m_schedule = HookSchedule::Release;

    closeSocket();
}

int lms_verify_hooks::read_header()
{
    int ret = ERROR_SUCCESS;

    if (m_schedule != HookSchedule::Header) {
        return ret;
    }

    int len = getBufferLen();

    MemoryChunk *buf = DMemPool::instance()->getMemory(len);
    DAutoFree(MemoryChunk, buf);

    if (copyData(buf->data, len) < 0) {
        ret = ERROR_TCP_SOCKET_COPY_FAILED;
        log_error("hook verify copy http header data filed. ret=%d", ret);
        return ret;
    }

    buf->length = len;

    if (m_parser->parse(buf->data, buf->length) < 0) {
        ret = ERROR_HTTP_HEADER_PARSER;
        log_error("hook verify parse http header failed. ret=%d", ret);
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
                log_error("hook verify read http header for reduce data failed. ret=%d", ret);
                return ret;
            }
        }

        duint16 status_code = m_parser->statusCode();

        if (status_code != 200) {
            log_error("hook verify error code is not 200");
            release();
        } else {
            m_content_length = m_parser->feild("Content-Length").toInt();
            DString val = m_parser->feild("Transfer-Encoding");
            m_reader->set_chunked((val == "chunked") ? true : false);
        }

        m_schedule = HookSchedule::Body;
    }

    return ret;
}

int lms_verify_hooks::read_body()
{
    int ret = ERROR_SUCCESS;

    if (m_schedule != HookSchedule::Body) {
        return ret;
    }

    if ((ret = m_reader->grow()) != ERROR_SUCCESS) {
        if (ret == ERROR_HTTP_READ_EOF || m_reader->get_body_len() == m_content_length) {
            m_schedule = HookSchedule::Completed;
            return ERROR_SUCCESS;
        }
        log_error("hook verify read http body failed. ret=%d", ret);
        return ret;
    }

    if (m_reader->get_body_len() == m_content_length) {
        m_schedule = HookSchedule::Completed;
    }

    return ret;
}

int lms_verify_hooks::parse_body()
{
    int ret = ERROR_SUCCESS;

    if (m_schedule != HookSchedule::Completed) {
        return ret;
    }

    int len = m_reader->get_body_len();

    MemoryChunk *chunk = DMemPool::instance()->getMemory(len);
    DAutoFree(MemoryChunk, chunk);

    if (m_reader->read_body(chunk->data, len) < 0) {
        ret = ERROR_HTTP_READ_BODY;
        log_error("hook verify read response failed. ret=%d", ret);
        return ret;
    }

    DString str(chunk->data, len);
    str = str.trimmed();
    if (!str.contains("{")) {
        ret = ERROR_RESPONSE_CODE;
        log_error("hook verify response not json. ret=%d", ret);
        return ret;
    }

    rapidjson::Document doc;
    doc.Parse(str.c_str());

    if (doc.HasParseError()) {
        ret = ERROR_RESPONSE_CODE;
        log_error("hook verify parse response json error. ret=%d", ret);
        return ret;
    }

    if (!doc.HasMember("code")) {
        ret = ERROR_RESPONSE_CODE;
        log_error("hook verify invalid response without code. ret=%d", ret);
        return ret;
    }

    rapidjson::Value& val = doc["code"];
    if (!val.IsInt()) {
        ret = ERROR_RESPONSE_CODE;
        log_error("hook verify response code is not number. ret=%d", ret);
        return ret;
    }

    int code = val.GetInt();

    if (code != ERROR_SUCCESS) {
        ret = ERROR_RESPONSE_CODE;
        log_error("hook verify error response code=%d. ret=%d", code, ret);
        return ret;
    }

    m_result = true;
    release();

    return ret;
}

void lms_verify_hooks::verify()
{
    if (m_handler) {
        m_handler(m_result);
        m_handler = NULL;
    }
}
