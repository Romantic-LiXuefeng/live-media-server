#ifndef LMS_RTMP_CLIENT_PLAY_HPP
#define LMS_RTMP_CLIENT_PLAY_HPP

#include "lms_client_stream_base.hpp"
#include "rtmp_client.hpp"

class lms_rtmp_client_play : public lms_client_stream_base
{
public:
    lms_rtmp_client_play(DEvent *event, lms_source *source);
    ~lms_rtmp_client_play();

    void set_connect_packet(ConnectAppPacket *pkt);

protected:
    virtual int start_handler();
    virtual int do_process();

private:
    rtmp_client *m_rtmp;

};

#endif // LMS_RTMP_CLIENT_PLAY_HPP
