#include "lms_source.hpp"
#include "kernel_codec.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"
#include "kernel_request.hpp"
#include "lms_threads_server.hpp"
#include "DGlobal.hpp"
#include "lms_edge.hpp"
#include <algorithm>

lms_source::lms_source(kernel_request *req)
    : m_publish(NULL)
    , m_play(NULL)
    , m_can_publish(true)
    , m_is_edge(false)
{
    m_req = new kernel_request();
    m_req->copy(req);

    m_gop_cache = new lms_gop_cache();

    m_external = new lms_source_external(m_req);
}

lms_source::~lms_source()
{
    DFree(m_req);
    DFree(m_gop_cache);
    DFree(m_publish);
    DFree(m_play);
    DFree(m_external);

    log_warn("-------------> free lms_source");
}

void lms_source::reload()
{
    DSpinLocker locker(&m_mutex);

    std::map<int, lms_reload_conn*>::iterator it;
    for (it = m_reloads.begin(); it != m_reloads.end(); ++it) {
        lms_reload_conn *conn = it->second;
        conn->write(1);
    }

    if (m_publish) {
        m_publish->reload();
    }

    if (m_play) {
        m_play->reload();
    }

    m_external->reload(m_gop_cache->video_sequence(), m_gop_cache->audio_sequence());
}

bool lms_source::reset()
{
    bool ret = m_external->reset();

    if (m_play || m_publish || !m_conns.empty() || !m_reloads.empty()) {
        ret = false;
    }

    return ret;
}

bool lms_source::onPlay(DEvent *event, bool edge)
{
    DSpinLocker locker(&m_mutex);

    int ret = ERROR_SUCCESS;

    m_is_edge = edge;

    if (m_is_edge && !m_play) {
        m_play = new lms_edge(this, event, false);
        if ((ret = m_play->start(m_req)) != ERROR_SUCCESS) {
            log_error("edge play start failed. ret=%d", ret);
            DFree(m_play);
            return false;
        }
    }

    return true;
}

bool lms_source::onPublish(DEvent *event, bool edge)
{
    DSpinLocker locker(&m_mutex);

    int ret = ERROR_SUCCESS;

    if (!m_can_publish) {
        log_error("someone has published, can not publish");
        return false;
    }

    m_is_edge = edge;
    m_can_publish = false;

    if (m_is_edge && !m_publish) {
        m_publish = new lms_edge(this, event, true);
        if ((ret = m_publish->start(m_req)) != ERROR_SUCCESS) {
            log_error("edge publish start failed. ret=%d", ret);
            DFree(m_publish);
            return false;
        }
    }

    return true;
}

void lms_source::onUnpublish()
{
    DSpinLocker locker(&m_mutex);

    m_can_publish = true;

    m_gop_cache->clear();

    if (m_is_edge && m_publish) {
        DFree(m_publish);
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

    m_external->onVideo(&video);

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

    m_external->onAudio(&audio);

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

    m_external->onMetadata(&metadata);

    return ret;
}

bool lms_source::add_connection(lms_conn_base *conn)
{
    DSpinLocker locker(&m_mutex);

    lms_event_conn *ev = NULL;
    std::map<pthread_t, lms_event_conn*>::iterator it = m_conns.find(conn->getThread());

    if (it == m_conns.end()) {
        ev = new lms_event_conn(conn->getEvent());

        if (!ev->open()) {
            log_error("event_conn add to event failed");
            ev->close();
            return false;
        }

        m_conns[conn->getThread()] = ev;
    } else {
        ev = it->second;
    }

    ev->addConnection(conn);

    return true;
}

void lms_source::del_connection(lms_conn_base *conn)
{
    DSpinLocker locker(&m_mutex);

    std::map<pthread_t, lms_event_conn*>::iterator it = m_conns.find(conn->getThread());

    if (it != m_conns.end()) {
        lms_event_conn *ev = it->second;
        ev->delConnection(conn);

        if (ev->empty()) {
            m_conns.erase(it);
            ev->close();
        }
    }

    if (m_conns.empty()) {
        if (m_is_edge) {
            m_gop_cache->clear();

            if (m_is_edge && m_play) {
                DFree(m_play);
            }
        }
    }
}

int lms_source::proxyMessage(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    DSpinLocker locker(&m_mutex);

    if (msg->payload->length <= 0) {
        return ret;
    }

    CommonMessage _msg(msg);

    if (m_is_edge && m_publish) {
        m_publish->process(&_msg);
    }

    return ret;
}

int lms_source::send_gop_cache(lms_stream_writer *writer, dint64 length)
{
    DSpinLocker locker(&m_mutex);

    return writer->send_gop_messages(m_gop_cache, length);
}

void lms_source::start_external()
{
    m_external->start();
}

void lms_source::stop_external()
{
    m_external->stop();
}

bool lms_source::add_reload_conn(lms_conn_base *base)
{
    DSpinLocker locker(&m_mutex);

    DEvent *event = base->getEvent();
    lms_reload_conn *conn = NULL;

    std::map<int, lms_reload_conn*>::iterator it = m_reloads.find(event->GetDescriptor());
    if (it == m_reloads.end()) {
        conn = new lms_reload_conn(event);

        if (!conn->open()) {
            log_error("reload_conn add to event failed");
            conn->close();
            return false;
        }

        m_reloads[event->GetDescriptor()] = conn;
    } else {
        conn = it->second;
    }

    conn->addConnection(base);

    return true;
}

void lms_source::del_reload_conn(lms_conn_base *base)
{
    DSpinLocker locker(&m_mutex);

    DEvent *event = base->getEvent();

    std::map<int, lms_reload_conn*>::iterator it = m_reloads.find(event->GetDescriptor());
    if (it != m_reloads.end()) {
        lms_reload_conn *conn = it->second;
        conn->delConnection(base);

        if (conn->empty()) {
            m_reloads.erase(it);
            conn->close();
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

lms_source *lms_source_manager::addSource(kernel_request *req)
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

    return source;
}

void lms_source_manager::reload()
{
    DSpinLocker locker(&m_mutex);

    std::map<DString, lms_source*>::iterator it;
    for (it = m_sources.begin(); it != m_sources.end(); ++it) {
        lms_source *source = it->second;
        source->reload();
    }

}

void lms_source_manager::reset()
{
    DSpinLocker locker(&m_mutex);

    std::map<DString, lms_source*>::iterator it;
    for (it = m_sources.begin(); it != m_sources.end();) {
        lms_source *source = it->second;
        if (source->reset()) {
            DFree(source);
            m_sources.erase(it++);
        } else {
            it++;
        }
    }
}
