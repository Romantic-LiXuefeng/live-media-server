#ifndef LMS_SOURCE_EXTERNAL_HPP
#define LMS_SOURCE_EXTERNAL_HPP

#include "lms_source_factory.hpp"

class lms_source_external
{
public:
    lms_source_external(kernel_request *req);
    ~lms_source_external();

    void start();
    void stop();
    void reload(CommonMessage *video_sh, CommonMessage *audio_sh);
    bool reset();
    virtual void onVideo(CommonMessage *msg);
    virtual void onAudio(CommonMessage *msg);
    virtual void onMetadata(CommonMessage *msg);

private:
    void init_hls();

private:
    kernel_request *m_req;
    lms_source_abstract_product *m_hls;

};

#endif // LMS_SOURCE_EXTERNAL_HPP
