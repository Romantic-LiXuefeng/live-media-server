#include "lms_dvr_flv.hpp"
#include "lms_config.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"
#include "kernel_codec.hpp"
#include <sys/time.h>

static const duint32 flv_header_size = 13;
static const char flv_header[flv_header_size] = { 'F', 'L', 'V', 0x01, 0x05, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00 };

lms_dvr_flv::lms_dvr_flv()
    : lms_source_abstract_product()
    , m_req(NULL)
    , m_time_expired(0)
    , m_correct(false)
    , m_started(false)
    , m_number(0)
    , m_temp_file(NULL)
    , m_fragment(0)
    , m_start_pts(0)
    , m_has_video(false)
    , m_video_sh(NULL)
    , m_audio_sh(NULL)
    , m_metadata(NULL)
{
    m_jitter = new lms_timestamp();
    m_jitter->set_correct_type(LmsTimeStamp::middle);

    m_elapsed = new DElapsedTimer();
}

lms_dvr_flv::~lms_dvr_flv()
{
    DFree(m_jitter);
    DFree(m_elapsed);
    DFree(m_temp_file);
    DFree(m_video_sh);
    DFree(m_audio_sh);
    DFree(m_metadata);
}

void lms_dvr_flv::start()
{
    if (m_started) {
        return;
    }

    lms_server_config_struct *config = lms_config::instance()->get_server(m_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config->get_flv_enable(m_req)) {
        return;
    }

    m_correct = config->get_flv_time_jitter(m_req);
    int jitter_type = config->get_flv_time_jitter_type(m_req);
    m_jitter->set_correct_type(jitter_type);

    m_time_expired = config->get_flv_time_expired(m_req);

    m_elapsed->stop();
    m_started = true;

    m_path = config->get_flv_path(m_req);
    m_root = config->get_flv_root(m_req);
    m_temp_filename = m_root + "/" + m_req->vhost + "/" + m_req->app + "/" + m_req->stream + "/" + m_req->stream + ".tempflv";
    m_fragment = config->get_flv_fragment(m_req) * 1000;

    reset_temp_file();
}

void lms_dvr_flv::stop()
{
    if (!m_started) {
        return;
    }

    rename_flv();

    DFree(m_video_sh);
    DFree(m_audio_sh);
    DFree(m_metadata);

    m_started = false;
    m_elapsed->start();
}

void lms_dvr_flv::reload()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config || !config->get_flv_enable(m_req)) {
        stop();
        return;
    }

    if (!m_started) {
        start();
        return;
    }

    m_time_expired = config->get_flv_time_expired(m_req);

    m_path = config->get_flv_path(m_req);
    m_root = config->get_flv_root(m_req);
    m_temp_filename = m_root + "/" + m_req->vhost + "/" + m_req->app + "/" + m_req->stream + "/" + m_req->stream + ".tempflv";
    m_fragment = config->get_flv_fragment(m_req) * 1000;

    if (rename_flv() != ERROR_SUCCESS) {
        return;
    }

    if (reset_temp_file() != ERROR_SUCCESS) {
        return;
    }
}

void lms_dvr_flv::reset()
{
    m_correct = false;
    m_started = false;

    m_number = 0;

    m_start_pts = 0;
    m_has_video = false;

    m_jitter->reset();
}

bool lms_dvr_flv::timeExpired()
{
    return m_elapsed->hasExpired(m_time_expired * 1000 * 1000);
}

void lms_dvr_flv::setRequest(kernel_request *req)
{
    m_req = req;
}

int lms_dvr_flv::onVideo(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if (!m_started) {
        return ret;
    }

    m_has_video = true;

    if (msg->is_sequence_header()) {
        DFree(m_video_sh);
        m_video_sh = new CommonMessage(msg);
    }

    bool keyframe = kernel_codec::video_is_keyframe(msg->payload->data, msg->payload_length);

    if ((ret = reap_segment(msg->dts, keyframe)) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = writeMessage(msg)) != ERROR_SUCCESS) {
        return ret;
    }

    return ret;
}

int lms_dvr_flv::onAudio(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if (!m_started) {
        return ret;
    }

    if (msg->is_sequence_header()) {
        DFree(m_audio_sh);
        m_audio_sh = new CommonMessage(msg);
    }

    if ((ret = reap_segment(msg->dts, false)) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = writeMessage(msg)) != ERROR_SUCCESS) {
        return ret;
    }

    return ret;
}

int lms_dvr_flv::onMetadata(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if (!m_started) {
        return ret;
    }

    DFree(m_metadata);
    m_metadata = new CommonMessage(msg);

    if ((ret = reap_segment(msg->dts, false)) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = writeMessage(msg)) != ERROR_SUCCESS) {
        return ret;
    }

    return ret;
}

int lms_dvr_flv::writeMessage(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    duint32 len = 11 + 4 + msg->payload->length;

    MemoryChunk *chunk = DMemPool::instance()->getMemory(len);
    DSharedPtr<MemoryChunk> packet = DSharedPtr<MemoryChunk>(chunk);
    packet->length = len;

    char *p = packet->data;

    // tag header: tag type.
    if (msg->is_video()) {
        *p++ = 0x09;
    } else if (msg->is_audio()) {
        *p++ = 0x08;
    } else if (msg->is_metadata()) {
        *p++ = 0x12;
    }

    // tag header: tag data size.
    char *pp = (char*)&msg->payload_length;
    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];

    // tag header: tag timestamp.
    pp = (char*)&msg->dts;
    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];
    *p++ = pp[3];

    // tag header: stream id always 0.
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x00;

    memcpy(p, msg->payload->data, msg->payload->length);
    p += msg->payload->length;

    // previous tag size.
    int pre_tag_len = msg->payload_length + 11;

    pp = (char*)&pre_tag_len;
    *p++ = pp[3];
    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];

    if (m_temp_file->write(packet->data, packet->length) != packet->length) {
        ret = ERROR_WRITE_FILE;
        log_error("write flv tag to temp file failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

DString lms_dvr_flv::generate_flv_filename()
{
    DString filename = m_path;

    struct timeval tv;
    gettimeofday(&tv, NULL);

    struct tm *tm;
    if ((tm = localtime(&tv.tv_sec)) == NULL) {
        return DString::number(m_number++) + ".flv";
    }

    DString yyyy = DString().sprintf("%d",   1900 + tm->tm_year);
    DString MM   = DString().sprintf("%02d", 1 + tm->tm_mon);
    DString dd   = DString().sprintf("%02d", tm->tm_mday);
    DString hh   = DString().sprintf("%02d", tm->tm_hour);
    DString mm   = DString().sprintf("%02d", tm->tm_min);
    DString ss   = DString().sprintf("%02d", tm->tm_sec);

    filename.replace("[yyyy]", yyyy, true);
    filename.replace("[MM]", MM, true);
    filename.replace("[dd]", dd, true);
    filename.replace("[hh]", hh, true);
    filename.replace("[mm]", mm, true);
    filename.replace("[ss]", ss, true);

    filename.replace("[vhost]", m_req->vhost, true);
    filename.replace("[app]", m_req->app, true);
    filename.replace("[stream]", m_req->stream, true);

    filename.replace("[num]",DString::number(m_number++), true);

    if (!filename.endWith(".flv")) {
        filename.append(".flv");
    }

    return filename;
}

int lms_dvr_flv::reset_temp_file()
{
    int ret = ERROR_SUCCESS;

    DString temp_dir = DFile::filePath(m_temp_filename);
    if (!create_dir(temp_dir)) {
        ret = ERROR_CREATE_DIR;
        log_error("create dir %s failed. ret=%d", temp_dir.c_str(), ret);
        return ret;
    }

    DFree(m_temp_file);
    m_temp_file = new DFile(m_temp_filename);
    if (!m_temp_file->open("w")) {
        ret = ERROR_OPEN_FILE;
        log_error("open temp %s failed. ret=%d", m_temp_filename.c_str(), ret);
        return ret;
    }

    if (m_temp_file->write(flv_header, flv_header_size) != flv_header_size) {
        ret = ERROR_WRITE_FILE;
        log_error("write flv header to temp file failed. temp_file=%s, ret=%d", m_temp_filename.c_str(), ret);
        return ret;
    }

    if (m_metadata) {
        if ((ret = writeMessage(m_metadata)) != ERROR_SUCCESS) {
            return ret;
        }
    }

    if (m_video_sh) {
        if ((ret = writeMessage(m_video_sh)) != ERROR_SUCCESS) {
            return ret;
        }
    }

    if (m_audio_sh) {
        if ((ret = writeMessage(m_audio_sh)) != ERROR_SUCCESS) {
            return ret;
        }
    }

    return ret;
}

int lms_dvr_flv::rename_flv()
{
    int ret = ERROR_SUCCESS;

    DFree(m_temp_file);

    DString filename = m_root + "/" + m_req->vhost + "/" + m_req->app + "/" + m_req->stream + "/" + generate_flv_filename();

    DString temp_dir = DFile::filePath(filename);
    if (!create_dir(temp_dir)) {
        ret = ERROR_CREATE_DIR;
        log_error("create dir %s failed. ret=%d", temp_dir.c_str(), ret);
        return ret;
    }

    if (::rename(m_temp_filename.c_str(), filename.c_str()) < 0) {
        ret = ERROR_RENAME_FILE;
        log_error("rename flv file failed. %s => %s, ret=%d", m_temp_filename.c_str(), filename.c_str(), ret);
        return ret;
    }

    return ret;
}

bool lms_dvr_flv::create_dir(const DString &dir)
{
    if (!DDir::exists(dir)) {
        if (!DDir::createDir(dir)) {
            return false;
        }
    }
    return true;
}

int lms_dvr_flv::reap_segment(int64_t pts, bool keyframe)
{
    int ret = ERROR_SUCCESS;

    if (m_start_pts == 0) {
        m_start_pts = pts;
    }

    dint64 duration = pts - m_start_pts;
    if (duration < m_fragment) {
        return ret;
    }

    if (!keyframe && m_has_video) {
        if (duration < m_fragment + 10000) {
            return ret;
        }
    }

    if ((ret = rename_flv()) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = reset_temp_file()) != ERROR_SUCCESS) {
        return ret;
    }

    m_start_pts = pts;
    m_has_video = false;

    return ret;
}
