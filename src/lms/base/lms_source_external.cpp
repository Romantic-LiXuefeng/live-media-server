#include "lms_source_external.hpp"
#include "lms_hls.hpp"
#include "lms_dvr_flv.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"
#include "lms_config.hpp"

lms_source_external::lms_source_external(kernel_request *req)
    : m_req(req)
{
    init_hls();
    init_flv();
}

lms_source_external::~lms_source_external()
{
    DFree(m_hls);
    DFree(m_flv);
}

void lms_source_external::start()
{
    m_hls->start();
    m_flv->start();
}

void lms_source_external::stop()
{
    m_hls->stop();
    m_flv->stop();
}

void lms_source_external::reload(CommonMessage *video_sh, CommonMessage *audio_sh)
{
    m_hls->reload();
    m_flv->reload();

    if (video_sh) {
        onVideo(video_sh);
    }
    if (audio_sh) {
        onAudio(audio_sh);
    }
}

bool lms_source_external::reset()
{
    int num = 0;

    if (m_hls->timeExpired()) {
        m_hls->reset();
        num++;
    }

    if (m_flv->timeExpired()) {
        m_flv->reset();
        num++;
    }

    return num == 2;
}

void lms_source_external::onVideo(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if ((ret = m_hls->onVideo(msg)) != ERROR_SUCCESS) {
        log_error("hls onVideo failed. ret=%d", ret);
        m_hls->stop();
    }

    if ((ret = m_flv->onVideo(msg)) != ERROR_SUCCESS) {
        log_error("flv onVideo failed. ret=%d", ret);
        m_flv->stop();
    }
}

void lms_source_external::onAudio(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if ((ret = m_hls->onAudio(msg)) != ERROR_SUCCESS) {
        log_error("hls onAudio failed. ret=%d", ret);
        m_hls->stop();
    }

    if ((ret = m_flv->onAudio(msg)) != ERROR_SUCCESS) {
        log_error("flv onAudio failed. ret=%d", ret);
        m_flv->stop();
    }
}

void lms_source_external::onMetadata(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if ((ret = m_flv->onMetadata(msg)) != ERROR_SUCCESS) {
        log_error("flv onMetadata failed. ret=%d", ret);
        m_flv->stop();
    }
}

void lms_source_external::init_hls()
{
    lms_source_abstract_factory *factory = new lms_hls_factory();
    m_hls = factory->createProduct();
    m_hls->setRequest(m_req);
    DFree(factory);
}

void lms_source_external::init_flv()
{
    lms_source_abstract_factory *factory = new lms_flv_factory();
    m_flv = factory->createProduct();
    m_flv->setRequest(m_req);
    DFree(factory);
}
