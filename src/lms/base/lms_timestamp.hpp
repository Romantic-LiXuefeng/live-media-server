#ifndef LMS_TIMESTAMP_HPP
#define LMS_TIMESTAMP_HPP

#include "DGlobal.hpp"
#include "kernel_global.hpp"

namespace LmsTimeStamp {
    enum CorrectType { simple = 0, middle, high };
}

/**
 * @brief
 */
class lms_timestamp
{
public:
    lms_timestamp();
    ~lms_timestamp();

    dint64 correct(CommonMessage *msg);

    void set_correct_type(int type);
    void set_correct_info(int v_delta, int a_delta);

    void reset();

private:
    dint64 high_correct(CommonMessage *msg);
    dint64 middle_correct(CommonMessage *msg);
    dint64 simple_correct(CommonMessage *msg);

private:
    int m_type;

private:
    dint64 m_last_audio;
    dint64 m_last_video;

    dint64 m_video_delta;
    dint64 m_audio_delta;

    dint64 m_origin_video;
    dint64 m_origin_audio;

    bool m_set_info;

    bool m_video_first;
    bool m_audio_first;

private:
    dint64 m_simple_last_pkt;
    dint64 m_simple_last_correct;

private:
    dint64 m_middle_last_pkt;
    dint64 m_middle_last_correct;

};

#endif // LMS_TIMESTAMP_HPP
