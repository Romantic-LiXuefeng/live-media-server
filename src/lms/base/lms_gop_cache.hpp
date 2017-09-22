#ifndef LMS_GOP_CACHE_HPP
#define LMS_GOP_CACHE_HPP

#include <deque>
#include "DSharedPtr.hpp"
#include "kernel_global.hpp"
#include "lms_timestamp.hpp"

class lms_gop_cache
{
public:
    lms_gop_cache();
    ~lms_gop_cache();

    void cache_metadata(CommonMessage *msg);
    void cache_video_sh(CommonMessage *msg);
    void cache_audio_sh(CommonMessage *msg);

    void cache(CommonMessage *_msg);
    void add(CommonMessage *_msg);

    void dump(std::deque<CommonMessage*> &_msgs, dint64 length,
              CommonMessage *&_metadata, CommonMessage *&_video_sh, CommonMessage *&_audio_sh);

    void CopyTo(lms_gop_cache *gop);

    void clear();

    CommonMessage *video_sequence();
    CommonMessage *audio_sequence();

private:
    void clear_first_gop();
    void clear_front(dint64 first);

public:
    typedef struct _GopMessage
    {
        CommonMessage *msg;
        dint64 correct_time;
    }GopMessage;

    std::deque<GopMessage> msgs;
    CommonMessage *metadata;
    CommonMessage *video_sh;
    CommonMessage *audio_sh;

    lms_timestamp *m_jitter;
    int m_gop_count;

    std::deque<dint64> m_durations;
    dint64 m_begin;
    dint64 m_end;

    bool m_first;
};

#endif // LMS_GOP_CACHE_HPP
