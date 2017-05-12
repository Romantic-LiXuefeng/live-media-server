#ifndef LMS_SOURCE_HPP
#define LMS_SOURCE_HPP

#include "DString.hpp"
#include "DSpinLock.hpp"
#include "rtmp_global.hpp"
#include "lms_gop_cache.hpp"
#include "lms_event_conn.hpp"
#include "lms_reload_conn.hpp"

#include "lms_play_edge.hpp"
#include "lms_publish_edge.hpp"

#include <pthread.h>
#include <map>
#include <deque>

class rtmp_request;

class lms_source
{
public:
    lms_source(rtmp_request *req);
    ~lms_source();

    void reload();

public:
    bool onPlay(DEvent *event);
    bool onPublish(DEvent *event);
    void onUnpublish();

    int onVideo(CommonMessage *msg);
    int onAudio(CommonMessage *msg);
    int onMetadata(CommonMessage *msg);

    void add_connection(lms_conn_base *conn);
    void del_connection(lms_conn_base *conn);

    int proxyMessage(CommonMessage *msg);

    void addEvent(DEvent *event);

public:
    void add_reload(lms_conn_base *base);
    void del_reload(lms_conn_base *base);

private:
    lms_event_conn *createEventConn(lms_conn_base *conn);

    void copy_gop(lms_event_conn *ev);

    bool get_config_value();

    void add_reload_conn(lms_conn_base *base);
    void del_reload_conn(lms_conn_base *base);

private:
    rtmp_request *m_req;

    std::map<pthread_t, lms_event_conn*> m_conns;

    // 防止同时推一路流，所以加锁控制
    bool m_can_publish;

    // 连接加入、删除、唤醒等需要用锁控制
    DSpinLock m_mutex;

    lms_gop_cache *m_gop_cache;

private:
    bool m_is_edge;
    lms_play_edge *m_play;
    lms_publish_edge *m_publish;

private:
    std::deque<lms_reload_conn*> m_reloads;

};

class lms_source_manager
{
public:
    lms_source_manager();
    ~lms_source_manager();

    static lms_source_manager *instance();
    /**
     * @brief 如果找到了就返回，没找到就new一个同时存起来
     */
    lms_source *addSource(rtmp_request *req);

    void reload();

private:
    static lms_source_manager *m_instance;

private:
    std::map<DString, lms_source*> m_sources;
    DSpinLock m_mutex;
};

#endif // LMS_SOURCE_HPP
