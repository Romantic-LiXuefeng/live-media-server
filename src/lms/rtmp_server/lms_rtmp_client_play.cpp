#include "lms_rtmp_client_play.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

lms_rtmp_client_play::lms_rtmp_client_play(DEvent *event, lms_source *source)
    : lms_client_stream_base(event, source)
{
    m_rtmp = new rtmp_client(this);
    m_rtmp->set_av_handler(AV_Handler_CALLBACK(&lms_rtmp_client_play::onMessage));
    m_rtmp->set_metadata_handler(AV_Handler_CALLBACK(&lms_rtmp_client_play::onMessage));

    m_rtmp->set_chunk_size(60000);
    m_rtmp->set_buffer_length(10000);

    setRecvTimeOut(m_timeout);
    setSendTimeOut(m_timeout);
}

lms_rtmp_client_play::~lms_rtmp_client_play()
{
    log_warn("free --> lms_rtmp_client_play");
    DFree(m_rtmp);
}

void lms_rtmp_client_play::set_connect_packet(ConnectAppPacket *pkt)
{
    m_rtmp->set_connect_packet(pkt);
}

int lms_rtmp_client_play::start_handler()
{
    int ret = ERROR_SUCCESS;

    m_rtmp->set_timeout(m_timeout);
    if ((ret = m_rtmp->start_rtmp(false, m_req)) != ERROR_SUCCESS) {
        log_error("rtmp ply client start failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int lms_rtmp_client_play::do_process()
{
    return m_rtmp->do_process();
}
