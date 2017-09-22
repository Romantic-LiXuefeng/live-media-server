#include "lms_gop_cache.hpp"
#include "kernel_codec.hpp"
#include "kernel_log.hpp"

#define GOP_CACHE_MAX_DURATION    10000

lms_gop_cache::lms_gop_cache()
    : metadata(NULL)
    , video_sh(NULL)
    , audio_sh(NULL)
{
    m_jitter = new lms_timestamp();
    m_jitter->set_correct_type(LmsTimeStamp::middle);

    m_gop_count = 0;
    m_begin = m_end = 0;
    m_first = true;
}

lms_gop_cache::~lms_gop_cache()
{
    clear();
    DFree(m_jitter);
}

void lms_gop_cache::cache_metadata(CommonMessage *msg)
{
    DFree(metadata);
    metadata = new CommonMessage(msg);
}

void lms_gop_cache::cache_video_sh(CommonMessage *msg)
{
    DFree(video_sh);
    video_sh = new CommonMessage(msg);
}

void lms_gop_cache::cache_audio_sh(CommonMessage *msg)
{
    DFree(audio_sh);
    audio_sh = new CommonMessage(msg);
}

void lms_gop_cache::cache(CommonMessage *_msg)
{
    if (_msg->is_video()) {
        if (_msg->is_sequence_header()) {
            cache_video_sh(_msg);
        } else {
            add(_msg);
        }
    } else if (_msg->is_audio()) {
        if (_msg->is_sequence_header()) {
            cache_audio_sh(_msg);
        } else {
            add(_msg);
        }
    } else if (_msg->is_metadata()) {
        cache_metadata(_msg);
    }
}

void lms_gop_cache::add(CommonMessage *_msg)
{
    CommonMessage *msg = new CommonMessage(_msg);

    dint64 dts = m_jitter->correct(msg);
    bool key_frame = false;

    if (msg->is_video() && msg->is_keyframe()) {
        key_frame = true;
    }

    if (!m_first) {
        GopMessage first = msgs.front();
        dint64 delta = dts - first.correct_time;

        if (delta > GOP_CACHE_MAX_DURATION) {
            if (m_gop_count >= 2) {
                dint64 first_duration = m_durations.front();
                if (delta - first_duration >= GOP_CACHE_MAX_DURATION) {
                    clear_first_gop();
                }
            } else if (m_gop_count == 1) {
                CommonMessage *temp = first.msg;
                if (!temp->is_video() || !temp->is_keyframe()) {
                    DFree(temp);
                    msgs.pop_front();

                    if (!key_frame) {
                        GopMessage gop_msg;
                        gop_msg.correct_time = dts;
                        gop_msg.msg = msg;
                        msgs.push_back(gop_msg);
                        return;
                    }
                } else {
                    if (delta > GOP_CACHE_MAX_DURATION * 2) {
                        clear_front(first.correct_time);
                    }
                }
            } else {
                DFree(first.msg);
                msgs.pop_front();
            }
        }
    } else {
        m_first = false;
    }

    if (key_frame) {
        dint64 duration = m_end - m_begin;
        m_begin = dts;
        if (duration > 0) {
            m_durations.push_back(duration);
        }

        m_gop_count++;
    }

    m_end = dts;

    GopMessage gop_msg;
    gop_msg.correct_time = dts;
    gop_msg.msg = msg;
    msgs.push_back(gop_msg);
}

void lms_gop_cache::dump(std::deque<CommonMessage*> &_msgs, dint64 length,
                         CommonMessage *&_metadata, CommonMessage *&_video_sh, CommonMessage *&_audio_sh)
{
    if (metadata) {
        _metadata = new CommonMessage(metadata);
    }
    if (video_sh) {
        _video_sh = new CommonMessage(video_sh);
    }
    if (audio_sh) {
        _audio_sh = new CommonMessage(audio_sh);
    }

    if (msgs.empty()) {
        return;
    }

    GopMessage g_msg = msgs.back();
    CommonMessage *msg = g_msg.msg;
    dint64 end_time = g_msg.correct_time;
    int pos = -1;

    for (int i = msgs.size() - 1; i >= 0; --i) {
        g_msg = msgs.at(i);
        msg = g_msg.msg;

        if (msg->is_video() && msg->is_keyframe()) {
            if (end_time - g_msg.correct_time >= length) {
                pos = i;
                break;
            }
        }    
    }

    pos = (pos == -1) ? 0 : pos;

    for (int i = pos; i < (int)msgs.size(); ++i) {
        g_msg = msgs.at(i);
        msg = new CommonMessage(g_msg.msg);
        _msgs.push_back(msg);
    }
}

void lms_gop_cache::CopyTo(lms_gop_cache *gop)
{
    if (metadata) {
        gop->cache_metadata(metadata);
    }
    if (video_sh) {
        gop->cache_video_sh(video_sh);
    }
    if (audio_sh) {
        gop->cache_audio_sh(audio_sh);
    }

    for (int i = 0; i < (int)msgs.size(); ++i) {
        GopMessage gop_msg = msgs.at(i);
        gop->cache(gop_msg.msg);
    }
}

void lms_gop_cache::clear()
{
    for (int i = 0; i < (int)msgs.size(); ++i) {
        GopMessage msg = msgs.at(i);
        DFree(msg.msg);
    }
    msgs.clear();

    DFree(metadata);
    DFree(video_sh);
    DFree(audio_sh);

    m_first = true;

    m_gop_count = 0;
    m_begin = m_end = 0;

    m_jitter->reset();
    m_durations.clear();
}

CommonMessage *lms_gop_cache::video_sequence()
{
    return video_sh;
}

CommonMessage *lms_gop_cache::audio_sequence()
{
    return audio_sh;
}

void lms_gop_cache::clear_first_gop()
{
    std::deque<GopMessage>::iterator it;
    CommonMessage *msg = NULL;
    int num = 0;

    for (it = msgs.begin(); it != msgs.end(); ++it) {
        GopMessage temp = *it;
        msg = temp.msg;

        if (num != 0 && msg->is_video() && msg->is_keyframe()) {
            break;
        }

        DFree(msg);
        num++;
    }

    msgs.erase(msgs.begin(), msgs.begin() + num);

    m_gop_count--;
    m_durations.pop_front();
}

void lms_gop_cache::clear_front(dint64 first)
{
    std::deque<GopMessage>::iterator it;
    int num = 0;

    for (it = msgs.begin(); it != msgs.end(); ++it) {
        GopMessage m = *it;
        if (m.correct_time - first >= 1000) {
            break;
        }

        DFree(m.msg);
        num++;
    }

    msgs.erase(msgs.begin(), msgs.begin() + num);
}
