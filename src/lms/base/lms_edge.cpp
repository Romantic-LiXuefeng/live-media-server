#include "lms_edge.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"
#include "lms_config.hpp"
#include <algorithm>
#include "lms_source.hpp"

lms_edge::lms_edge(lms_source *source, DEvent *event, bool publish)
    : m_event(event)
    , m_source(source)
    , m_publish(publish)
    , m_rtmp_publish(NULL)
    , m_flv_publish(NULL)
    , m_rtmp_play(NULL)
    , m_flv_play(NULL)
    , m_ts_play(NULL)
    , m_ts_publish(NULL)
    , m_type(Rtmp)
    , m_started(false)
    , m_pos(0)
{
    m_src_req = new kernel_request();
    m_dst_req = new kernel_request();

    m_gop_cache = new lms_gop_cache();
}

lms_edge::~lms_edge()
{
    stop();

    DFree(m_src_req);
    DFree(m_dst_req);
    DFree(m_gop_cache);
}

int lms_edge::start(kernel_request *req)
{
    int ret = ERROR_SUCCESS;

    if (m_started) {
        return ret;
    }

    m_src_req->copy(req);
    m_dst_req->copy(req);

    if (!get_config_value()) {
        ret = ERROR_GET_CONFIG_VALUE;
        return ret;
    }

    m_started = true;

    get_ip_port();

    switch (m_type) {
    case Rtmp:
        start_rtmp();
        break;
    case Flv:
        start_flv();
        break;
    case Ts:
        start_ts();
        break;
    default:
        break;
    }

    return ret;
}

void lms_edge::stop()
{
    m_started = false;
    m_pos = 0;

    m_event->delReadTimeOut(this);

    switch (m_type) {
    case Rtmp:
        if (m_rtmp_publish) {
            m_rtmp_publish->release();
        }
        if (m_rtmp_play) {
            m_rtmp_play->release();
        }
        break;
    case Flv:
        if (m_flv_publish) {
            m_flv_publish->release();
        }
        if (m_flv_play) {
            m_flv_play->release();
        }
        break;
    case Ts:
        if (m_ts_publish) {
            m_ts_publish->release();
        }
        if (m_ts_play) {
            m_ts_play->release();
        }
        break;
    default:
        break;
    }
}

void lms_edge::release()
{
    m_rtmp_publish = NULL;
    m_flv_publish = NULL;
    m_rtmp_play = NULL;
    m_flv_play = NULL;
    m_ts_play = NULL;
    m_ts_publish = NULL;

    if (m_started) {
        if (m_pos >= (int)m_proxy_pass.size()) {
            // 全部轮询一遍之后等待一秒再重新轮询
            m_event->addReadTimeOut(this, 1 * 1000 * 1000);
        } else {
            retry();
        }
    }
}

void lms_edge::reload()
{
    bool need_retry = false;

    lms_server_config_struct *config = lms_config::instance()->get_server(m_src_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        switch (m_type) {
        case Rtmp:
            if (m_rtmp_publish) {
                m_rtmp_publish->release();
            }
            if (m_rtmp_play) {
                m_rtmp_play->release();
            }
            break;
        case Flv:
            if (m_flv_publish) {
                m_flv_publish->release();
            }
            if (m_flv_play) {
                m_flv_play->release();
            }
            break;
        case Ts:
            if (m_ts_publish) {
                m_ts_publish->release();
            }
            if (m_ts_play) {
                m_ts_play->release();
            }
            break;
        default:
            break;
        }

        return;
    }

    m_proxy_pass = config->get_proxy_pass(m_src_req);

    std::vector<DString>::iterator it = find(m_proxy_pass.begin(), m_proxy_pass.end(), m_host);
    if (it == m_proxy_pass.end()) {
        need_retry = true;
    }

    DString proxy_type = config->get_proxy_type(m_src_req);
    dint8 type;
    if (proxy_type == "rtmp") {
        type = Rtmp;
    } else if (proxy_type == "flv") {
        type = Flv;
    } else if (proxy_type == "ts") {
        type = Ts;
    }

    if (m_type != type) {
        need_retry = true;
    }
    m_type = type;

    DString vhost = config->get_proxy_vhost(m_src_req);
    DString app = config->get_proxy_app(m_src_req);
    DString stream = config->get_proxy_stream(m_src_req);

    if (vhost.contains("[vhost]")) {
        if (m_src_req->vhost == DEFAULT_VHOST) {
            vhost.replace("[vhost]", m_src_req->host);
        } else {
            vhost.replace("[vhost]", m_src_req->vhost);
        }
    }

    if (app.contains("[app]")) {
        app.replace("[app]", m_src_req->app);
    }

    if (stream.contains("[stream]")) {
        stream.replace("[stream]", m_src_req->stream);
    }

    if ((vhost != m_dst_req->vhost) || (app != m_dst_req->app) || (stream != m_dst_req->stream)) {
        need_retry = true;
    }

    m_dst_req->vhost = vhost;
    m_dst_req->app = app;
    m_dst_req->stream = stream;
    m_dst_req->tcUrl = "rtmp://" + m_dst_req->vhost + "/" + m_dst_req->app;

    if (need_retry) {
        switch (m_type) {
        case Rtmp:
            if (m_rtmp_publish) {
                m_rtmp_publish->release();
            }
            if (m_rtmp_play) {
                m_rtmp_play->release();
            }
            break;
        case Flv:
            if (m_flv_publish) {
                m_flv_publish->release();
            }
            if (m_flv_play) {
                m_flv_play->release();
            }
            break;
        case Ts:
            if (m_ts_publish) {
                m_ts_publish->release();
            }
            if (m_ts_play) {
                m_ts_play->release();
            }
            break;
        default:
            break;
        }
    } else {
        switch (m_type) {
        case Rtmp:
            if (m_rtmp_publish) {
                m_rtmp_publish->reload();
            }
            if (m_rtmp_play) {
                m_rtmp_play->reload();
            }
            break;
        case Flv:
            if (m_flv_publish) {
                m_flv_publish->reload();
            }
            if (m_flv_play) {
                m_flv_play->reload();
            }
            break;
        case Ts:
            if (m_ts_publish) {
                m_ts_publish->reload();
            }
            if (m_ts_play) {
                m_ts_play->reload();
            }
            break;
        default:
            break;
        }
    }
}

void lms_edge::process(CommonMessage *msg)
{
    m_gop_cache->cache(msg);

    switch (m_type) {
    case Rtmp:
        if (m_rtmp_publish) {
            m_rtmp_publish->process(msg);
        }
        break;
    case Flv:
        if (m_flv_publish) {
            m_flv_publish->process(msg);
        }
        break;
    case Ts:
        if (m_ts_publish) {
            m_ts_publish->process(msg);
        }
        break;
    default:
        break;
    }
}

void lms_edge::onReadTimeOut()
{
    log_trace("publish edge retry");
    m_event->delReadTimeOut(this);
    retry();
}

void lms_edge::onWriteTimeOut()
{
    // nothing to do
}

bool lms_edge::get_config_value()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_src_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        return false;
    }

    m_proxy_pass = config->get_proxy_pass(m_src_req);
    if (m_proxy_pass.empty()) {
        return false;
    }

    DString proxy_type = config->get_proxy_type(m_src_req);
    if (proxy_type == "rtmp") {
        m_type = Rtmp;
    } else if (proxy_type == "flv") {
        m_type = Flv;
    } else if (proxy_type == "ts") {
        m_type = Ts;
    }

    DString vhost = config->get_proxy_vhost(m_src_req);
    DString app = config->get_proxy_app(m_src_req);
    DString stream = config->get_proxy_stream(m_src_req);

    if (vhost.contains("[vhost]")) {
        if (m_src_req->vhost == DEFAULT_VHOST) {
            vhost.replace("[vhost]", m_src_req->host);
        } else {
            vhost.replace("[vhost]", m_src_req->vhost);
        }
    }
    m_dst_req->vhost = vhost;

    if (app.contains("[app]")) {
        app.replace("[app]", m_src_req->app);
    }
    m_dst_req->app = app;

    if (stream.contains("[stream]")) {
        stream.replace("[stream]", m_src_req->stream);
    }
    m_dst_req->stream = stream;

    m_dst_req->tcUrl = "rtmp://" + m_dst_req->vhost + "/" + m_dst_req->app;

    return true;
}

void lms_edge::get_ip_port()
{
    switch (m_type) {
    case Rtmp:
        m_port = 1935;
        break;
    case Flv:
        m_port = 80;
        break;
    case Ts:
        m_port = 80;
        break;
    default:
        break;
    }

    if (m_pos >= (int)m_proxy_pass.size()) {
        m_pos = 0;
    }

    DString host = m_proxy_pass.at(m_pos);
    DStringList value = host.split(":");
    if (value.size() >= 2) {
        m_port = value.at(1).toInt();
    }

    m_ip = value.at(0);
    m_host = host;
    m_pos++;
}

void lms_edge::retry()
{    
    get_ip_port();

    switch (m_type) {
    case Rtmp:
        start_rtmp();
        break;
    case Flv:
        start_flv();
        break;
    case Ts:
        start_ts();
        break;
    default:
        break;
    }
}

void lms_edge::start_flv()
{
    if (m_publish) {
        m_flv_publish = new lms_http_client_flv_publish(this, m_event, m_gop_cache);
        m_flv_publish->start(m_src_req, m_dst_req, m_ip, m_port);
    } else {
        m_flv_play = new lms_http_client_flv_play(this, m_event, m_source);
        m_flv_play->start(m_src_req, m_dst_req, m_ip, m_port);
    }
}

void lms_edge::start_rtmp()
{
    if (m_publish) {
        m_rtmp_publish = new lms_rtmp_client_publish(this, m_event, m_gop_cache);
        m_rtmp_publish->start(m_src_req, m_dst_req, m_ip, m_port);
    } else {
        m_rtmp_play = new lms_rtmp_client_play(this, m_event, m_source);
        m_rtmp_play->start(m_src_req, m_dst_req, m_ip, m_port);
    }
}

void lms_edge::start_ts()
{
    if (m_publish) {
        m_ts_publish = new lms_http_client_ts_publish(this, m_event, m_gop_cache);
        m_ts_publish->start(m_src_req, m_dst_req, m_ip, m_port);
    } else {
        m_ts_play = new lms_http_client_ts_play(this, m_event, m_source);
        m_ts_play->start(m_src_req, m_dst_req, m_ip, m_port);
    }
}

