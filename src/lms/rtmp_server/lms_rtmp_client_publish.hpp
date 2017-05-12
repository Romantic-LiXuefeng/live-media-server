#ifndef LMS_RTMP_CLIENT_PUBLISH_HPP
#define LMS_RTMP_CLIENT_PUBLISH_HPP

#include "lms_client_stream_base.hpp"
#include "rtmp_client.hpp"

#include <vector>

class lms_rtmp_client_publish : public lms_client_stream_base
{
public:
    lms_rtmp_client_publish(DEvent *event, lms_source *source);
    ~lms_rtmp_client_publish();

protected:
    virtual int start_handler();
    virtual int do_process();
    virtual int send_av_data(CommonMessage *msg);
    virtual bool can_publish();

private:
    rtmp_client *m_rtmp;

};

#endif // LMS_RTMP_CLIENT_PUBLISH_HPP
