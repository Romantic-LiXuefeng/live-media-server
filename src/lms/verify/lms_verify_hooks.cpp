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

lms_verify_hooks::lms_verify_hooks(DEvent *event)
    : DTcpSocket(event)
    , m_content_length(-1)
    , m_handler(NULL)
    , m_result(false)
    , m_post_started(false)
    , m_timeout(DEFAULT_HOOK_TIMEOUT)
{
    m_reader = new http_reader(this, false, HTTP_HEADER_CALLBACK(&lms_verify_hooks::onHttpHeader));
}

lms_verify_hooks::~lms_verify_hooks()
{
    log_warn("free --> lms_verify_hooks");

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

void lms_verify_hooks::on_connect(kernel_request *req, duint64 id, const DString &url, const DString &ip)
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

void lms_verify_hooks::on_publish(kernel_request *req, duint64 id, const DString &url, const DString &ip)
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

void lms_verify_hooks::on_play(kernel_request *req, duint64 id, const DString &url, const DString &ip)
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

void lms_verify_hooks::on_unpublish(kernel_request *req, duint64 id, const DString &url, const DString &ip)
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

void lms_verify_hooks::on_stop(kernel_request *req, duint64 id, const DString &url, const DString &ip)
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

    if ((ret = m_reader->service()) != ERROR_SUCCESS) {
        if (ret != SOCKET_EAGAIN) {
            log_error("hook read http response failed. ret=%d", ret);
            return ret;
        }
    }

    return parse_body();
}

int lms_verify_hooks::onWriteProcess()
{
    global_context->update_id(m_fd);

    return do_post();
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
    h->setErrorHandler(HOST_CALLBACK(&lms_verify_hooks::onHostError));

    if (!h->lookupHost(m_uri.get_host())) {
        h->close();
        release();
    }
}

void lms_verify_hooks::onHost(const DStringList &ips)
{
    int ret = ERROR_SUCCESS;

    global_context->update(m_id);

    DString ip = ips.at(0);
    int port = m_uri.get_port();

    if ((ret = connectToHost(ip.c_str(), port)) != ERROR_SUCCESS) {
        if (ret != SOCKET_EINPROGRESS) {
            ret = ERROR_TCP_SOCKET_CONNECT;
            log_error("hook verify connect to host failed. ip=%s, port=%d, ret=%d", ip.c_str(), port, ret);
            release();
        }
    }
}

void lms_verify_hooks::onHostError(const DStringList &ips)
{
    log_error("hook get host failed. host=%s, ips_count=%d", m_uri.get_host().c_str(), ips.size());
    release();
}

int lms_verify_hooks::do_post()
{
    int ret = ERROR_SUCCESS;

    if (m_post_started) {
        return ret;
    }

    global_context->set_id(m_fd);
    global_context->update_id(m_fd);

    log_trace("hook verify client_id, hook_id: %llu[%llu]", m_id, global_context->get_id());

    setReadTimeOut(m_timeout);
    setWriteTimeOut(m_timeout);

    DHttpHeader header;
    header.setContentLength(m_value.length());
    header.setHost(m_uri.get_host());

    DString value = header.getRequestString("POST", m_uri.get_path());

    value.append(m_value);

    MemoryChunk *chunk = DMemPool::instance()->getMemory(value.size());
    DSharedPtr<MemoryChunk> response = DSharedPtr<MemoryChunk>(chunk);

    memcpy(response->data, value.data(), value.size());
    response->length = value.size();

    m_post_started = true;

    if ((ret = write(response, response->length)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_HTTP_WRITE_VERIFY_VALUE, "write verify data failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

void lms_verify_hooks::release()
{
    global_context->update_id(m_fd);

    verify();

    global_context->delete_id(m_fd);

    close();
}

int lms_verify_hooks::parse_body()
{
    int ret = ERROR_SUCCESS;

    if (m_reader->getLength() < m_content_length) {
        return ret;
    }

    MemoryChunk *chunk = DMemPool::instance()->getMemory(m_content_length);
    DAutoFree(MemoryChunk, chunk);

    if (m_reader->readBody(chunk->data, m_content_length) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_HTTP_READ_BODY, "hook verify read response failed. ret=%d", ret);
        return ret;
    }

    DString str(chunk->data, m_content_length);
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

int lms_verify_hooks::onHttpHeader(DHttpParser *parser)
{
    int ret = ERROR_SUCCESS;

    duint16 status_code = parser->statusCode();

    if (status_code != 200) {
        ret = ERROR_HTTP_VERIFY_STATUS_CODE;
        log_error("hook verify error code is not 200. ret=%d", ret);
        return ret;
    } else {
        DString ch = parser->feild("Content-Length");
        if (!ch.empty()) {
            m_content_length = ch.toInt();
        } else {
            ret = ERROR_HTTP_VERIFY_NO_CONTENT_LENGTH;
            log_error("hook verify no Content-Length");
            return ret;
        }
    }

    return ret;
}
