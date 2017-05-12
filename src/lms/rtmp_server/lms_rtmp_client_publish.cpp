#include "lms_rtmp_client_publish.hpp"
#include "kernel_errno.hpp"
#include "DGlobal.hpp"
#include "kernel_log.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

lms_rtmp_client_publish::lms_rtmp_client_publish(DEvent *event, lms_source *source)
    : lms_client_stream_base(event, source)
{
    m_rtmp = new rtmp_client(this);
    m_rtmp->set_chunk_size(60000);

    setRecvTimeOut(m_timeout);
    setSendTimeOut(m_timeout);
}

lms_rtmp_client_publish::~lms_rtmp_client_publish()
{
    log_warn("free --> lms_rtmp_client_publish");

    DFree(m_rtmp);
}

int lms_rtmp_client_publish::start_handler()
{
    int ret = ERROR_SUCCESS;

    m_rtmp->set_timeout(m_timeout);
    if ((ret = m_rtmp->start_rtmp(true, m_req)) != ERROR_SUCCESS) {
        log_error("rtmp publish client start failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int lms_rtmp_client_publish::do_process()
{
    return m_rtmp->do_process();
}

int lms_rtmp_client_publish::send_av_data(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if ((ret = m_rtmp->send_av_data(msg)) != ERROR_SUCCESS) {
        log_error("rtmp publish client send audio/video message failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

bool lms_rtmp_client_publish::can_publish()
{
    return m_rtmp->can_publish();
}
