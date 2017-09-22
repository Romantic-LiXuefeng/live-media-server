#include "lms_source_external.hpp"
#include "lms_hls.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"
#include "lms_config.hpp"

lms_source_external::lms_source_external(kernel_request *req)
    : m_req(req)
{
    init_hls();
}

lms_source_external::~lms_source_external()
{
    DFree(m_hls);
}

void lms_source_external::start()
{
    m_hls->start();
}

void lms_source_external::stop()
{
    m_hls->stop();
}

void lms_source_external::reload(CommonMessage *video_sh, CommonMessage *audio_sh)
{
    m_hls->reload();

    if (video_sh) {
        onVideo(video_sh);
    }
    if (audio_sh) {
        onAudio(audio_sh);
    }
}

bool lms_source_external::reset()
{
    bool time_expired = true;

    if (m_hls->timeExpired()) {
        m_hls->reset();
    } else {
        time_expired = false;
    }

    return time_expired;
}

void lms_source_external::onVideo(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if ((ret = m_hls->onVideo(msg)) != ERROR_SUCCESS) {
        log_error("hls onVideo failed. ret=%d", ret);
        m_hls->stop();
    }
}

void lms_source_external::onAudio(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if ((ret = m_hls->onAudio(msg)) != ERROR_SUCCESS) {
        log_error("hls onAudio failed. ret=%d", ret);
        m_hls->stop();
    }
}

void lms_source_external::onMetadata(CommonMessage *msg)
{

}

void lms_source_external::init_hls()
{
    lms_source_abstract_factory *factory = new lms_hls_factory();
    m_hls = factory->createProduct();
    m_hls->setRequest(m_req);
    DFree(factory);
}
