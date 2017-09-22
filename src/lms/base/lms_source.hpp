#ifndef LMS_SOURCE_HPP
#define LMS_SOURCE_HPP

#include "DString.hpp"
#include "DSpinLock.hpp"
#include "kernel_global.hpp"
#include "lms_gop_cache.hpp"
#include "lms_event_conn.hpp"
#include "lms_reload_conn.hpp"
#include "lms_stream_writer.hpp"
#include "lms_source_external.hpp"

#include <pthread.h>
#include <map>
#include <deque>

class kernel_request;
class lms_edge;

class lms_source
{
public:
    lms_source(kernel_request *req);
    ~lms_source();

    void reload();
    bool reset();

public:
    bool onPlay(DEvent *event, bool edge);
    bool onPublish(DEvent *event, bool edge);
    void onUnpublish();

    int onVideo(CommonMessage *msg);
    int onAudio(CommonMessage *msg);
    int onMetadata(CommonMessage *msg);

    bool add_connection(lms_conn_base *conn);
    void del_connection(lms_conn_base *conn);

    int proxyMessage(CommonMessage *msg);

    int send_gop_cache(lms_stream_writer *writer, dint64 length);

    // external function contain hls, dvr, dash, ...
    void start_external();
    void stop_external();

public:
    bool add_reload_conn(lms_conn_base *base);
    void del_reload_conn(lms_conn_base *base);

private:
    kernel_request *m_req;
    lms_gop_cache *m_gop_cache;

    lms_edge *m_publish;
    lms_edge *m_play;

    // 防止同时推一路流，所以加锁控制
    bool m_can_publish;

    // 连接加入、删除、唤醒等需要用锁控制
    DSpinLock m_mutex;

    bool m_is_edge;

    lms_source_external *m_external;

private:
    std::map<pthread_t, lms_event_conn*> m_conns;
    std::map<int, lms_reload_conn*> m_reloads;

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
    lms_source *addSource(kernel_request *req);

    void reload();

    void reset();

private:
    static lms_source_manager *m_instance;

private:
    std::map<DString, lms_source*> m_sources;
    DSpinLock m_mutex;
};

#endif // LMS_SOURCE_HPP
