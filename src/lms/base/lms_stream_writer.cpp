#include "lms_stream_writer.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"
#include "kernel_codec.hpp"
#include "lms_config.hpp"
#include "DTcpSocket.hpp"

lms_stream_writer::lms_stream_writer(kernel_request *req, AVHandler handler, bool edge)
    : m_req(req)
    , m_handler(handler)
    , m_is_edge(edge)
    , m_correct(true)
    , m_queue_size(30 * 1024 * 1024)
    , m_cache_size(0)
    , m_fast_enable(false)
    , m_gop_enable(true)
{
    m_jitter = new lms_timestamp();

    get_config_value();
}

lms_stream_writer::~lms_stream_writer()
{
    DFree(m_jitter);

    clear();
}

int lms_stream_writer::send(CommonMessage *msg)
{
    if (m_cache_size >= m_queue_size) {
        reduce();
    }

    m_cache_size += msg->payload->length;

    dint64 timestamp = m_jitter->correct(msg);

    CommonMessage *_msg = new CommonMessage(msg);
    if (m_correct) {
        _msg->dts = timestamp;
    }
    m_msgs.push_back(_msg);

    return flush();
}

int lms_stream_writer::flush()
{
    int ret = ERROR_SUCCESS;

    while (!m_msgs.empty()) {
        CommonMessage *msg = m_msgs.front();
        DAutoFree(CommonMessage, msg);

        m_msgs.pop_front();

        if ((ret = m_handler(msg)) != ERROR_SUCCESS) {
            break;
        }
    }

    return ret;
}

void lms_stream_writer::reload()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        return;
    }

    m_fast_enable = config->get_fast_gop(m_req);
    m_gop_enable = config->get_gop_cache(m_req);

    int queue_size = config->get_queue_size(m_req);
    m_queue_size = queue_size * 1024 * 1024;
}

int lms_stream_writer::send_gop_messages(lms_gop_cache *gop, dint64 length)
{
    CommonMessage *metadata = NULL;
    CommonMessage *video_sh = NULL;
    CommonMessage *audio_sh = NULL;

    dint64 first_time = 0;

    std::deque<CommonMessage*> msgs;
    gop->dump(msgs, length, metadata, video_sh, audio_sh);

    if (!m_gop_enable && !m_fast_enable) {
        for (int i = 0; i < (int)msgs.size(); ++i) {
            DFree(msgs.at(i));
        }
        msgs.clear();
    } else if (!msgs.empty()) {
        dint64 timestamp = 0;
        CommonMessage *msg = msgs.back();

        for (int i = 0; i < (int)msgs.size(); ++i) {
            msg = msgs.at(i);
            timestamp = m_jitter->correct(msg);

            if (m_correct) {
                msg->dts = timestamp;
            }

            if (i == 0) {
                first_time = timestamp;
            }
        }

        dint64 audio_start_time = msg->dts - length;

        for (int i = 0; i < (int)msgs.size(); ++i) {
            msg = msgs.at(i);
            if (m_fast_enable) {
                if (msg->is_audio() && audio_start_time > msg->dts) {
                    continue;
                }
            }
            m_msgs.push_back(new CommonMessage(msg));
        }

        for (int i = 0; i < (int)msgs.size(); ++i) {
            DFree(msgs.at(i));
        }
        msgs.clear();
    }

    if (audio_sh) {
        audio_sh->dts = first_time;
        m_msgs.push_front(audio_sh);
    }
    if (video_sh) {
        video_sh->dts = first_time;
        m_msgs.push_front(video_sh);
    }
    if (metadata) {
        metadata->dts = first_time;
        m_msgs.push_front(metadata);
    }

    return flush();
}

void lms_stream_writer::get_config_value()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        return;
    }

    m_fast_enable = config->get_fast_gop(m_req);
    m_gop_enable = config->get_gop_cache(m_req);

    int queue_size = config->get_queue_size(m_req);
    m_queue_size = queue_size * 1024 * 1024;

    if (!m_is_edge) {
        m_correct = config->get_time_jitter(m_req);

        int jitter_type = config->get_time_jitter_type(m_req);
        m_jitter->set_correct_type(jitter_type);
    }
}

void lms_stream_writer::clear()
{
    for (int i = 0; i < (int)m_msgs.size(); ++i) {
        DFree(m_msgs.at(i));
    }
    m_msgs.clear();

    m_cache_size = 0;
}

void lms_stream_writer::reduce()
{
    while (!m_msgs.empty()) {
        CommonMessage *msg = m_msgs.front();

        if (msg->is_video() && msg->is_keyframe()) {
            if (m_cache_size < m_queue_size){
                break;
            }
        }

        m_cache_size -= msg->payload->length;
        m_msgs.pop_front();
        DFree(msg);
    }
}
