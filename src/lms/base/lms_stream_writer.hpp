#ifndef LMS_STREAM_WRITER_HPP
#define LMS_STREAM_WRITER_HPP

#include "kernel_global.hpp"
#include "kernel_request.hpp"
#include "lms_timestamp.hpp"
#include "lms_gop_cache.hpp"
#include "DTcpSocket.hpp"
#include <deque>

class lms_stream_writer
{
public:
    lms_stream_writer(kernel_request *req, AVHandler handler, bool edge);
    ~lms_stream_writer();

    int send(CommonMessage *msg);
    int flush();

    void reload();

    int send_gop_messages(lms_gop_cache *gop, dint64 length);

private:
    void get_config_value();
    void clear();
    void reduce();

private:
    kernel_request *m_req;
    lms_timestamp *m_jitter;
    AVHandler m_handler;

private:
    bool m_is_edge;

    bool m_correct;

    dint64 m_queue_size;
    // 当前链接中缓存数据的大小
    dint64 m_cache_size;

    // fast gop enable
    bool m_fast_enable;
    // gop cache enable
    bool m_gop_enable;

    std::deque<CommonMessage*> m_msgs;

};

#endif // LMS_STREAM_WRITER_HPP
