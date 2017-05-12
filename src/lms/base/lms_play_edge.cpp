#include "lms_play_edge.hpp"
#include "rtmp_global.hpp"
#include "lms_config.hpp"
#include "kernel_log.hpp"
#include "kernel_errno.hpp"
#include <algorithm>

lms_play_edge::lms_play_edge(lms_source *source, DEvent *event)
    : m_source(source)
    , m_event(event)
    , m_rtmp_play(NULL)
    , m_flv_play(NULL)
    , m_pass_pos(0)
    , m_started(false)
    , m_timeout(10 * 1000 * 1000)
{
    m_req = new rtmp_request();
    m_base_req = new rtmp_request;
}

lms_play_edge::~lms_play_edge()
{
    DFree(m_req);
    DFree(m_base_req);
    stop();
}

int lms_play_edge::start(rtmp_request *req)
{
    int ret = ERROR_SUCCESS;

    if (m_started) {
        return ret;
    }

    m_req->copy(req);
    m_base_req->copy(req);

    // 如果获取配置失败，此时需要停止回源功能
    if (get_config_value(req) != ERROR_SUCCESS) {
        ret = ERROR_GET_CONFIG_VALUE;
        log_error("proxy pass config parse failed. ret=%d", ret);
        return ret;
    }

    DString ip;
    int port;

    if (!get_ip_port(ip, port)) {
        ret = ERROR_EGDE_GET_IP_PORT;
        log_error("play edge all proxy_pass are invalid. ret=%d", ret);
        return ret;
    }

    if (m_proxy_type == "rtmp") {
        start_rtmp(ip, port);
    } else if (m_proxy_type == "flv") {
        start_flv(ip, port);
    }

    log_trace("play edge start success. ip=%s, port=%d", ip.c_str(), port);
    m_started = true;

    return ret;
}

void lms_play_edge::stop()
{
    m_started = false;
    m_pass_pos = 0;

    m_event->delReadTimeOut(this);

    if (m_proxy_type == "rtmp") {
        if (m_rtmp_play) {
            m_rtmp_play->release();
        }
    } else if (m_proxy_type == "flv") {
        if (m_flv_play) {
            m_flv_play->release();
        }
    }
}

void lms_play_edge::reload()
{
    if (!m_started) {
        return;
    }

    log_trace("play edge start reload");

    lms_server_config_struct *config = lms_config::instance()->get_server(m_base_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        return;
    }

    bool need_reload = false;

    DString proxy_type = config->get_proxy_type(m_base_req);

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

void lms_play_edge::onReadTimeOut()
{
    retry();
}

void lms_play_edge::onWriteTimeOut()
{
    // nothing to do
}

void lms_play_edge::onRelease()
{
    m_rtmp_play = NULL;
    m_flv_play = NULL;

    if (m_started) {
        m_event->addReadTimeOut(this, 1 * 1000 * 1000);
    }
}

void lms_play_edge::start_flv(const DString &ip, int port)
{
    m_flv_play = new lms_http_client_flv_play(m_event, m_source);
    m_flv_play->set_timeout(m_timeout);
    m_flv_play->setReleaseHandler(RELEASE_CALLBACK(&lms_play_edge::onRelease));
    m_flv_play->start(m_req, ip, port);
    log_trace("play edge retry flv. ip=%s, port=%d", ip.c_str(), port);
}

void lms_play_edge::start_rtmp(const DString &ip, int port)
{
    m_rtmp_play = new lms_rtmp_client_play(m_event, m_source);
    m_rtmp_play->set_timeout(m_timeout);
    m_rtmp_play->setReleaseHandler(RELEASE_CALLBACK(&lms_play_edge::onRelease));
    m_rtmp_play->start(m_req, ip, port);
    log_trace("play edge retry rtmp. ip=%s, port=%d", ip.c_str(), port);
}

bool lms_play_edge::get_ip_port(DString &ip, int &port)
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

int lms_play_edge::get_config_value(rtmp_request *req)
{
    int ret = ERROR_SUCCESS;

    lms_server_config_struct *server_config = lms_config::instance()->get_server(req);
    DAutoFree(lms_server_config_struct, server_config);

    if (!server_config) {
        ret = ERROR_SERVER_CONFIG_EMPTY;
        log_error("play edge server config is empty. ret=%d", ret);
        return ret;
    }

    m_proxy_type = server_config->get_proxy_type(req);

    DString vhost = "[vhost]";
    DString app = "[app]";
    DString stream = "[stream]";

    vhost = server_config->get_proxy_vhost(req);
    app = server_config->get_proxy_app(req);
    stream = server_config->get_proxy_stream(req);

    int timeout = server_config->get_proxy_timeout(req);
    m_timeout = timeout * 1000 * 1000;

    m_proxy_pass = server_config->get_proxy_pass(req);
    if (m_proxy_pass.empty()) {
        ret = ERROR_PROXY_PASS;
        log_error("play edge proxy_pass is empty. ret=%d", ret);
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

    log_trace("play edge get config value success. vhost=%s, app=%s, stream=%s", vhost.c_str(), app.c_str(), stream.c_str());

    return ret;
}

void lms_play_edge::retry()
{
    DString ip;
    int port;
    get_ip_port(ip, port);

    if (m_proxy_type == "rtmp") {
        start_rtmp(ip, port);
    } else if (m_proxy_type == "flv") {
        start_flv(ip, port);
    }
}

