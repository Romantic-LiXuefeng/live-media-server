#include "lms_timestamp.hpp"
#include "kernel_codec.hpp"

#define BEGIN_THRESHOLD     -1000
#define END_THRESHOLD       1000

lms_timestamp::lms_timestamp()
    : m_type(LmsTimeStamp::middle)
    , m_last_audio(0)
    , m_last_video(0)
    , m_video_delta(0)
    , m_audio_delta(0)
    , m_origin_video(0)
    , m_origin_audio(0)
    , m_set_info(false)
    , m_video_first(true)
    , m_audio_first(true)
    , m_simple_last_pkt(0)
    , m_simple_last_correct(-1)
    , m_middle_last_pkt(0)
    , m_middle_last_correct(-1)
{

}

lms_timestamp::~lms_timestamp()
{

}

dint64 lms_timestamp::correct(CommonMessage *msg)
{
    dint64 timestamp = 0;

    switch (m_type) {
    case LmsTimeStamp::simple:
        timestamp = simple_correct(msg);
        break;
    case LmsTimeStamp::middle:
        timestamp = middle_correct(msg);
        break;
    case LmsTimeStamp::high:
        timestamp = high_correct(msg);
        break;
    default:
        timestamp = msg->dts;
        break;
    }

    return timestamp;
}

void lms_timestamp::set_correct_type(int type)
{
    m_type = type;
}

void lms_timestamp::set_correct_info(int v_delta, int a_delta)
{
    m_set_info = true;
    m_video_delta = v_delta;
    m_audio_delta = a_delta;
}

void lms_timestamp::reset()
{
    m_last_audio = 0;
    m_last_video = 0;

    m_video_delta = 0;
    m_audio_delta = 0;

    m_origin_video = 0;
    m_origin_audio = 0;

    m_set_info = false;

    m_video_first = true;
    m_audio_first = true;

    m_simple_last_pkt = 0;
    m_simple_last_correct = -1;

    m_middle_last_pkt = 0;
    m_middle_last_correct = -1;
}

dint64 lms_timestamp::high_correct(CommonMessage *msg)
{
    dint64 timestamp = 0;
    dint64 delta = 0;
    dint64 latest = DMax(m_last_audio, m_last_video);
    dint64 ori_v_delta, ori_a_delta;
    bool ori_normal = false;

    if (msg->is_video() && msg->is_sequence_header()) {
        if (m_video_first) {
            return 0;
        } else {
            return m_last_video + m_video_delta;
        }
    }

    if (msg->is_audio() && msg->is_sequence_header()) {
        if (m_audio_first) {
            return 0;
        } else {
            return m_last_audio + m_audio_delta;
        }
    }

    if (msg->is_video()) {
        delta = msg->dts - m_last_video;

        if (!m_set_info) {
            ori_v_delta = msg->dts - m_origin_video;
            if (ori_v_delta > 0 && ori_v_delta < END_THRESHOLD) {
                m_video_delta = ori_v_delta;
                ori_normal = true;
            }
        }
        m_origin_video = msg->dts;

        if (delta < BEGIN_THRESHOLD || delta > END_THRESHOLD) {
            m_last_video += m_video_delta;
        } else {
            if (!m_set_info && !ori_normal) {
                m_video_delta = delta;
            }

            if (m_video_first) {
                m_last_video = 0;
            } else {
                m_last_video += m_video_delta;
            }
        }

        dint64 temp = m_last_video - latest;
        if (temp < BEGIN_THRESHOLD || temp > END_THRESHOLD) {
            m_last_video = latest;
        }

        timestamp = m_last_video;
        m_video_first = false;
    }

    if (msg->is_audio()) {
        delta = msg->dts - m_last_audio;

        if (!m_set_info) {
            ori_a_delta = msg->dts - m_origin_audio;
            if (ori_a_delta > 0 && ori_a_delta < END_THRESHOLD) {
                m_audio_delta = ori_a_delta;
                ori_normal = true;
            }
        }
        m_origin_audio = msg->dts;

        if (delta < BEGIN_THRESHOLD || delta > END_THRESHOLD) {
            m_last_audio += m_audio_delta;
        } else {
            if (!m_set_info && !ori_normal) {
                m_audio_delta = delta;
            }

            if (m_audio_first) {
                m_last_audio = 0;
            } else {
                m_last_audio += m_audio_delta;
            }
        }

        dint64 temp = m_last_audio - latest;
        if (temp < BEGIN_THRESHOLD || temp > END_THRESHOLD) {
            m_last_audio = latest;
        }

        timestamp = m_last_audio;
        m_audio_first = false;
    }

    return timestamp;
}

dint64 lms_timestamp::middle_correct(CommonMessage *msg)
{
    dint64 timestamp = DMax(msg->dts, 0);

    if (msg->is_sequence_header()) {
        if (m_middle_last_correct == -1) {
            return 0;
        } else {
            m_middle_last_pkt = timestamp;
            return m_middle_last_correct;
        }
    }

    if (m_middle_last_correct == -1) {
        m_middle_last_correct = m_middle_last_pkt = 0;
        return 0;
    }

    dint64 delta = timestamp - m_middle_last_pkt;

    if (delta < BEGIN_THRESHOLD || delta > END_THRESHOLD) {
        m_middle_last_correct += 10;
    } else {
        m_middle_last_correct += delta;
    }

    m_middle_last_pkt = timestamp;

    return m_middle_last_correct;
}

dint64 lms_timestamp::simple_correct(CommonMessage *msg)
{
    dint64 timestamp = DMax(msg->dts, 0);

    if (msg->is_sequence_header()) {
        m_simple_last_pkt = timestamp;
        return timestamp;
    }

    if (m_simple_last_correct != -1) {
        dint64 delta = timestamp - m_simple_last_pkt;
        if (delta < BEGIN_THRESHOLD || delta > END_THRESHOLD) {
            m_simple_last_correct += 10;
        } else {
            m_simple_last_correct = timestamp;
        }
    } else {
        m_simple_last_correct = timestamp;
    }

    m_simple_last_pkt = timestamp;

    return m_simple_last_correct;
}
