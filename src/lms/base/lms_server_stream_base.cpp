#include "lms_server_stream_base.hpp"
#include "lms_conn_base.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"
#include "kernel_utility.hpp"

lms_server_stream_base::lms_server_stream_base(lms_conn_base *parent)
    : m_parent(parent)
    , m_source(NULL)
    , m_correct(true)
    , m_queue_size(30 * 1024 * 1024)
    , m_cache_size(0)
    , m_fast_enable(false)
    , m_gop_enable(true)
    , m_timeout(30 * 1000 * 1000)
    , m_socket_threshold(10 * 1024 * 1024)
{
    m_req = new rtmp_request();
    m_jitter = new lms_timestamp();

    m_client_ip = get_peer_ip(m_parent->GetDescriptor());
}

lms_server_stream_base::~lms_server_stream_base()
{
    clear_messages();
    DFree(m_req);
    DFree(m_jitter);
}

void lms_server_stream_base::reload()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        return;
    }

    int queue_size = config->get_queue_size(m_req);
    m_queue_size = queue_size * 1024 * 1024;
    m_socket_threshold = DMin(m_queue_size, 10 * 1024 * 1024);

    int timeout = config->get_timeout(m_req);
    m_timeout = timeout * 1000 * 1000;

    reload_self(config);
}

bool lms_server_stream_base::get_config_value(rtmp_request *req)
{
    lms_server_config_struct *config = lms_config::instance()->get_server(req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        return false;
    }

    m_fast_enable = config->get_fast_gop(req);
    m_correct = config->get_time_jitter(req);
    m_gop_enable = config->get_gop_cache(req);

    int queue_size = config->get_queue_size(req);
    m_queue_size = queue_size * 1024 * 1024;
    m_socket_threshold = DMin(m_queue_size, 10 * 1024 * 1024);

    int jitter_type = config->get_time_jitter_type(req);
    m_jitter->set_correct_type(jitter_type);

    int timeout = config->get_timeout(req);
    m_timeout = timeout * 1000 * 1000;

    return get_self_config(config);
}

int lms_server_stream_base::process(CommonMessage *_msg)
{
    if (m_cache_size + m_parent->getWriteBufferLen() >= m_queue_size) {
        clear_messages();
    }

    m_cache_size += _msg->payload->length;

    dint64 timestamp = m_jitter->correct(_msg);

    CommonMessage *msg = new CommonMessage(_msg);
    if (m_correct) {
        msg->header.timestamp = timestamp;
    }
    m_msgs.push_back(msg);

    return send_message();
}

int lms_server_stream_base::onMessage(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if (!m_source) {
        return ret;
    }

    // 回源转推
    if ((ret = m_source->proxyMessage(msg)) == ERROR_SERVER_PROXY_MESSAGE) {
        return ERROR_SUCCESS;
    }

    if (msg->is_audio()) {
        return m_source->onAudio(msg);
    } else if (msg->is_video()) {
        return m_source->onVideo(msg);
    } else {
        return m_source->onMetadata(msg);
    }

    return ret;
}

void lms_server_stream_base::clear_messages()
{
    for (int i = 0; i < (int)m_msgs.size(); ++i) {
        DFree(m_msgs.at(i));
    }
    m_msgs.clear();

    m_cache_size = 0;
}

int lms_server_stream_base::send_gop_messages(dint64 length)
{
    CommonMessage *metadata = NULL;
    CommonMessage *video_sh = NULL;
    CommonMessage *audio_sh = NULL;

    std::deque<CommonMessage*> msgs;
    m_parent->getGopCache()->dump(msgs, length, metadata, video_sh, audio_sh);

    dint64 first_time = 0;

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
                msg->header.timestamp = timestamp;
            }

            if (i == 0) {
                first_time = timestamp;
            }
        }

        dint64 audio_start_time = msg->header.timestamp - length;

        for (int i = 0; i < (int)msgs.size(); ++i) {
            msg = msgs.at(i);
            if (m_fast_enable) {
                if (msg->is_audio() && audio_start_time > msg->header.timestamp) {
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
        audio_sh->header.timestamp = first_time;
        m_msgs.push_front(audio_sh);
    }
    if (video_sh) {
        video_sh->header.timestamp = first_time;
        m_msgs.push_front(video_sh);
    }
    if (metadata) {
        metadata->header.timestamp = first_time;
        m_msgs.push_front(metadata);
    }

    return send_message();
}

int lms_server_stream_base::send_av_data(CommonMessage *msg)
{
    return ERROR_SUCCESS;
}

int lms_server_stream_base::send_message()
{
    int ret = ERROR_SUCCESS;

    if (m_parent->getWriteBufferLen() < m_socket_threshold) {
        for (int i = 0; i < (int)m_msgs.size(); ++i) {
            CommonMessage *msg = m_msgs.at(i);

            if ((ret = send_av_data(msg)) != ERROR_SUCCESS) {
                log_error("server stream base send audio/video data failed. ret=%d", ret);
                return ret;
            }
        }

        clear_messages();
    }

    if ((ret = m_parent->writeData()) != ERROR_SUCCESS) {
        ret = ERROR_TCP_SOCKET_WRITE_FAILED;
        log_error("server stream base write data failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

DString lms_server_stream_base::get_client_ip()
{
    return m_client_ip;
}
