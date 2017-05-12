#include "lms_publish_edge.hpp"
#include "lms_config.hpp"
#include "kernel_log.hpp"
#include "kernel_errno.hpp"
#include "kernel_codec.hpp"
#include <algorithm>

lms_publish_edge::lms_publish_edge(lms_source *source, DEvent *event)
    : m_source(source)
    , m_event(event)
    , m_rtmp_publish(NULL)
    , m_flv_publish(NULL)
    , m_pass_pos(0)
    , m_started(false)
    , m_buffer_size(30 * 1024 * 1024)
    , m_timeout(10 * 1000 * 1000)
    , m_http_chunked(true)
{
    m_req = new rtmp_request();
    m_base_req = new rtmp_request();
    m_gop_cache = new lms_gop_cache();
}

lms_publish_edge::~lms_publish_edge()
{
    stop();
    DFree(m_req);
    DFree(m_base_req);
    DFree(m_gop_cache);
}

int lms_publish_edge::start(rtmp_request *req)
{
    int ret = ERROR_SUCCESS;

    if (m_started) {
        return ret;
    }

    m_req->copy(req);
    m_base_req->copy(req);

    if (get_config_value(req) != ERROR_SUCCESS) {
        ret = ERROR_GET_CONFIG_VALUE;
        log_error("publish edge get config value failed. ret=%d", ret);
        return ret;
    }

    DString ip;
    int port;

    if (!get_ip_port(ip, port)) {
        ret = ERROR_EGDE_GET_IP_PORT;
        log_error("publish edge all proxy_pass are invalid. ret=%d", ret);
        return ret;
    }

    if (m_proxy_type == "rtmp") {
        start_rtmp(ip, port);
    } else if (m_proxy_type == "flv") {
        start_flv(ip, port);
    }

    m_started = true;

    return ret;
}

void lms_publish_edge::stop()
{
    m_started = false;
    m_pass_pos = 0;

    m_event->delReadTimeOut(this);

    if (m_proxy_type == "rtmp") {
        if (m_rtmp_publish) {
            m_rtmp_publish->release();
        }
    } else if (m_proxy_type == "flv") {
        if (m_flv_publish) {
            m_flv_publish->release();
        }
    }

    m_gop_cache->clear();
}

void lms_publish_edge::reload()
{
    if (!m_started) {
        return;
    }

    log_trace("publish edge start reload");

    lms_server_config_struct *config = lms_config::instance()->get_server(m_base_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        return;
    }

    bool need_reload = false;

    DString proxy_type = config->get_proxy_type(m_base_req);

    m_http_chunked = config->get_http_chunked(m_base_req);

    DString vhost = "[vhost]";
    DString app = "[app]";
    DString stream = "[stream]";

    vhost = config->get_proxy_vhost(m_base_req);
    app = config->get_proxy_app(m_base_req);
    stream = config->get_proxy_stream(m_base_req);

    std::vector<DString> proxy_pass = config->get_proxy_pass(m_base_req);
    if (proxy_pass.empty()) {
        log_error("proxy_pass is empty, reload is stopped");
        return;
    }

    if (proxy_pass.size() != m_proxy_pass.size()) {
        need_reload = true;
    } else {
        std::vector<DString>::iterator it;
        for (int i = 0; i < (int)proxy_pass.size(); ++i) {
            it = find(m_proxy_pass.begin(), m_proxy_pass.end(), proxy_pass.at(i));
            if (it == m_proxy_pass.end()) {
                need_reload = true;
                break;
            }
        }
    }

    if (vhost.contains("[vhost]")) {
        vhost.replace("[vhost]", m_base_req->vhost);
    }

    if (app.contains("[app]")) {
        app.replace("[app]", m_base_req->app);
    }

    if (stream.contains("[stream]")) {
        stream.replace("[stream]", m_base_req->stream);
    }

    if (proxy_type != m_proxy_type ||
            vhost != m_req->vhost ||
            app != m_req->app ||
            stream != m_req->stream)
    {
        need_reload = true;
    }

    if (need_reload) {
        stop();
        start(m_base_req);
    }
}

void lms_publish_edge::process(CommonMessage *_msg)
{
    m_gop_cache->cache(_msg);

    if (m_proxy_type == "rtmp") {
        if (m_rtmp_publish) {
            m_rtmp_publish->process(_msg);
        }
    } else if (m_proxy_type == "flv") {
        if (m_flv_publish) {
            m_flv_publish->process(_msg);
        }
    }
}

void lms_publish_edge::onReadTimeOut()
{
    retry();
}

void lms_publish_edge::onWriteTimeOut()
{
    // nothing to do
}

int lms_publish_edge::get_config_value(rtmp_request *req)
{
    int ret = ERROR_SUCCESS;

    lms_server_config_struct *server_config = lms_config::instance()->get_server(req);
    DAutoFree(lms_server_config_struct, server_config);

    if (!server_config) {
        ret = ERROR_SERVER_CONFIG_EMPTY;
        log_error("publish edge server config is empty. ret=%d", ret);
        return ret;
    }

    int buffer_size = server_config->get_queue_size(req);
    m_buffer_size = buffer_size * 1024 * 1024;

    int timeout = server_config->get_proxy_timeout(req);
    m_timeout = timeout * 1000 * 1000;

    m_proxy_type = server_config->get_proxy_type(req);

    DString vhost = server_config->get_proxy_vhost(req);
    DString app = server_config->get_proxy_app(req);
    DString stream = server_config->get_proxy_stream(req);

    m_proxy_pass = server_config->get_proxy_pass(req);
    if (m_proxy_pass.empty()) {
        ret = ERROR_PROXY_PASS;
        log_error("publish edge proxy_pass is empty. ret=%d", ret);
        return ret;
    }

    if (vhost.contains("[vhost]")) {
        vhost.replace("[vhost]", m_req->vhost);
    }
    m_req->vhost = vhost;

    if (app.contains("[app]")) {
        app.replace("[app]", m_req->app);
    }
    m_req->app = app;

    if (stream.contains("[stream]")) {
        stream.replace("[stream]", m_req->stream);
    }
    m_req->stream = stream;

    m_req->tcUrl = "rtmp://" + m_req->vhost + "/" + m_req->app;

    log_trace("publish edge start success. vhost=%s, app=%s, stream=%s", vhost.c_str(), app.c_str(), stream.c_str());

    return 0;
}

void lms_publish_edge::retry()
{
    DString ip;
    int port;
    get_ip_port(ip, port);

    if (m_proxy_type == "rtmp") {
        start_rtmp(ip, port);
    } else if (m_proxy_type == "flv") {
        start_flv(ip, port);
    }

    dump_gop_messages();
}

void lms_publish_edge::dump_gop_messages()
{
    CommonMessage *metadata = NULL;
    CommonMessage *video_sh = NULL;
    CommonMessage *audio_sh = NULL;

    std::deque<CommonMessage*> msgs;
    m_gop_cache->dump(msgs, 3000, metadata, video_sh, audio_sh);

    if (m_proxy_type == "rtmp" && m_rtmp_publish) {
        m_rtmp_publish->process(metadata);
        m_rtmp_publish->process(video_sh);
        m_rtmp_publish->process(audio_sh);

        for (int i = 0; i < (int)msgs.size(); ++i) {
            CommonMessage *msg = msgs.at(i);
            m_rtmp_publish->process(msg);
            DFree(msg);
        }
    } else if (m_proxy_type == "flv" && m_flv_publish) {
        m_flv_publish->process(metadata);
        m_flv_publish->process(video_sh);
        m_flv_publish->process(audio_sh);

        for (int i = 0; i < (int)msgs.size(); ++i) {
            CommonMessage *msg = msgs.at(i);
            m_flv_publish->process(msg);
            DFree(msg);
        }
    }

    DFree(metadata);
    DFree(video_sh);
    DFree(audio_sh);
}

void lms_publish_edge::start_flv(const DString &ip, int port)
{
    m_flv_publish = new lms_http_client_flv_publish(m_event, m_source);
    m_flv_publish->set_buffer_size(m_buffer_size);
    m_flv_publish->set_timeout(m_timeout);
    m_flv_publish->set_chunked(m_http_chunked);
    m_flv_publish->setReleaseHandler(RELEASE_CALLBACK(&lms_publish_edge::onRelease));
    m_flv_publish->start(m_req, ip, port);
}

void lms_publish_edge::start_rtmp(const DString &ip, int port)
{
    m_rtmp_publish = new lms_rtmp_client_publish(m_event, m_source);
    m_rtmp_publish->set_buffer_size(m_buffer_size);
    m_rtmp_publish->set_timeout(m_timeout);
    m_rtmp_publish->setReleaseHandler(RELEASE_CALLBACK(&lms_publish_edge::onRelease));
    m_rtmp_publish->start(m_req, ip, port);
}

void lms_publish_edge::onRelease()
{
    m_rtmp_publish = NULL;
    m_flv_publish = NULL;

    if (m_started) {
        m_event->addReadTimeOut(this, 1 * 1000 * 1000);
    }
}

bool lms_publish_edge::get_ip_port(DString &ip, int &port)
{
    if (m_proxy_type == "rtmp") {
        port = 1935;
    } else if (m_proxy_type == "flv") {
        port = 80;
    }

    int num = 0;

    while (true) {
        // 如果所有的proxy_pass均无法使用，此时回源功能需要停止
        if (num >= (int)m_proxy_pass.size()) {
            return false;
        }

        if (m_pass_pos >= (int)m_proxy_pass.size()) {
            m_pass_pos = 0;
        }

        DString host = m_proxy_pass.at(m_pass_pos);
        m_pass_pos++;

        DStringList value = host.split(":");
        if (value.size() > 2) {
            num++;
            continue;
        }

        ip = value.at(0);
        if (value.size() == 2) {
            port = value.at(1).toInt();
        }

        break;
    }

    return true;
}

