#include "lms_source.hpp"
#include "kernel_codec.hpp"
#include "kernel_errno.hpp"
#include "lms_config.hpp"
#include "lms_threads_server.hpp"

#include "DGlobal.hpp"
#include "kernel_log.hpp"

#include <algorithm>

lms_source::lms_source(rtmp_request *req)
    : m_can_publish(true)
    , m_is_edge(false)
    , m_play(NULL)
    , m_publish(NULL)
{
    m_req = new rtmp_request();
    m_req->copy(req);

    m_gop_cache = new lms_gop_cache();
}

lms_source::~lms_source()
{
    DFree(m_req);
    DFree(m_gop_cache);
    DFree(m_play);
    DFree(m_publish);
}

void lms_source::reload()
{
    DSpinLocker locker(&m_mutex);

    for (int i = 0; i < (int)m_reloads.size(); ++i) {
        lms_reload_conn *conn = m_reloads.at(i);
        conn->write(1);
    }

    if (m_play) {
        m_play->reload();
    }

    if (m_publish) {
        m_publish->reload();
    }
}

bool lms_source::onPlay(DEvent *event)
{
    DSpinLocker locker(&m_mutex);

    if (!get_config_value()) {
        log_error("source get config value failed");
        return false;
    }

    if (m_is_edge) {
        if (!m_play) {
            m_play = new lms_play_edge(this, event);
        }
        m_play->start(m_req);
    }

    return true;
}

bool lms_source::onPublish(DEvent *event)
{
    DSpinLocker locker(&m_mutex);

    if (!m_can_publish) {
        return false;
    }

    m_can_publish = false;

    if (!get_config_value()) {
        log_error("source get config value failed");
        return false;
    }

    if (m_is_edge) {
        if (!m_publish) {
            m_publish = new lms_publish_edge(this, event);
        }
        m_publish->start(m_req);
    }

    return true;
}

void lms_source::onUnpublish()
{
    DSpinLocker locker(&m_mutex);

    m_can_publish = true;

    m_gop_cache->clear();

    if (m_is_edge) {
        if (m_publish) {
            m_publish->stop();
            m_publish = NULL;
        }
    }
}

int lms_source::onVideo(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    DSpinLocker locker(&m_mutex);

    if (msg->payload->length <= 0) {
        return ret;
    }

    CommonMessage video(msg);

    m_gop_cache->cache(&video);

    std::map<pthread_t, lms_event_conn*>::iterator it;
    for (it = m_conns.begin(); it != m_conns.end(); ++it) {
        lms_event_conn *conn = it->second;
        conn->addData(&video);
    }

    return ret;
}

int lms_source::onAudio(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    DSpinLocker locker(&m_mutex);

    if (msg->payload->length <= 0) {
        return ret;
    }

    CommonMessage audio(msg);

    m_gop_cache->cache(&audio);

    std::map<pthread_t, lms_event_conn*>::iterator it;
    for (it = m_conns.begin(); it != m_conns.end(); ++it) {
        lms_event_conn *conn = it->second;
        conn->addData(&audio);
    }

    return ret;
}

int lms_source::onMetadata(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    DSpinLocker locker(&m_mutex);

    if (msg->payload->length <= 0) {
        return ret;
    }

    CommonMessage metadata(msg);

    m_gop_cache->cache(&metadata);

    std::map<pthread_t, lms_event_conn*>::iterator it;
    for (it = m_conns.begin(); it != m_conns.end(); ++it) {
        lms_event_conn *conn = it->second;
        conn->addData(&metadata);
    }

    return ret;
}

void lms_source::add_connection(lms_conn_base *conn)
{
    DSpinLocker locker(&m_mutex);

    std::map<pthread_t, lms_event_conn*>::iterator it;
    lms_event_conn *ev = NULL;

    it = m_conns.find(conn->getThread());

    if (it == m_conns.end()) {
        ev = createEventConn(conn);
        copy_gop(ev);
        m_conns[conn->getThread()] = ev;
    } else {
        ev = it->second;
    }

    conn->setGopCache(ev->gop_cache);
    ev->addConnection(conn);
}

void lms_source::del_connection(lms_conn_base *conn)
{
    DSpinLocker locker(&m_mutex);

    std::map<pthread_t, lms_event_conn*>::iterator it;
    lms_event_conn *ev = NULL;

    it = m_conns.find(conn->getThread());

    if (it != m_conns.end()) {
        ev = it->second;
        ev->delConnection(conn);

        if (ev->empty()) {
            m_conns.erase(it);
            ev->close();
        }
    }

    if (m_conns.empty()) {
        if (m_is_edge) {
            m_gop_cache->clear();

            if (m_play) {
                m_play->stop();
                m_play = NULL;
            }
        }
    }
}

int lms_source::proxyMessage(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    DSpinLocker locker(&m_mutex);

    if (m_is_edge) {
        if (m_publish) {
            m_publish->process(msg);
        }
        return ERROR_SERVER_PROXY_MESSAGE;
    }

    return ret;
}

void lms_source::addEvent(DEvent *event)
{
    DSpinLocker locker(&m_mutex);

    lms_reload_conn *conn = new lms_reload_conn(event);
    conn->open();
    m_reloads.push_back(conn);
}

void lms_source::add_reload(lms_conn_base *base)
{
    DSpinLocker locker(&m_mutex);

    add_reload_conn(base);
}

void lms_source::del_reload(lms_conn_base *base)
{
    DSpinLocker locker(&m_mutex);

    del_reload_conn(base);
}

lms_event_conn *lms_source::createEventConn(lms_conn_base *conn)
{
    lms_event_conn *ev = new lms_event_conn(conn->getEvent());
    ev->open();
    return ev;
}

void lms_source::copy_gop(lms_event_conn *ev)
{
    m_gop_cache->CopyTo(ev->gop_cache);
}

bool lms_source::get_config_value()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        return false;
    }

    m_is_edge = config->get_proxy_enable(m_req);
    return true;
}

void lms_source::add_reload_conn(lms_conn_base *base)
{
    for (int i = 0; i < (int)m_reloads.size(); ++i) {
        lms_reload_conn *conn = m_reloads.at(i);

        if (conn->getEvent() == base->getEvent()) {
            conn->addConnection(base);
        }
    }
}

void lms_source::del_reload_conn(lms_conn_base *base)
{
    for (int i = 0; i < (int)m_reloads.size(); ++i) {
        lms_reload_conn *conn = m_reloads.at(i);

        if (conn->getEvent() == base->getEvent()) {
            conn->delConnection(base);
        }
    }
}

/*********************************************************************/

lms_source_manager *lms_source_manager::m_instance = new lms_source_manager;
lms_source_manager::lms_source_manager()
{

}

lms_source_manager::~lms_source_manager()
{
    std::map<DString, lms_source*>::iterator it;
    for (it = m_sources.begin(); it != m_sources.end(); ++it) {
        lms_source *source = it->second;
        DFree(source);
    }
    m_sources.clear();
}

lms_source_manager *lms_source_manager::instance()
{
    return m_instance;
}

lms_source *lms_source_manager::addSource(rtmp_request *req)
{
    DSpinLocker locker(&m_mutex);

    DString url = req->get_stream_url();

    std::map<DString, lms_source*>::iterator it;
    it = m_sources.find(url);
    if (it != m_sources.end()) {
        return it->second;
    }

    lms_source *source = new lms_source(req);
    m_sources[url] = source;

    std::vector<DTcpServer *> servers = lms_server_manager::instance()->getServer();
    for (int i = 0; i < (int)servers.size(); ++i) {
        DTcpServer *srv = servers.at(i);
        source->addEvent(srv->getEvent());
    }

    return source;
}

void lms_source_manager::reload()
{
    std::map<DString, lms_source*>::iterator it;
    for (it = m_sources.begin(); it != m_sources.end(); ++it) {
        lms_source *source = it->second;
        source->reload();
    }

}
