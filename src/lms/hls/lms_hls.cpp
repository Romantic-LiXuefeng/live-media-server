#include "lms_hls.hpp"
#include "lms_config.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"
#include "kernel_codec.hpp"
#include <sys/time.h>
#include <math.h>

lms_hls_product::lms_hls_product()
    : lms_source_abstract_product()
    , m_req(NULL)
    , m_muxer(NULL)
    , m_time_expired(0)
    , m_has_video(true)
    , m_has_audio(true)
    , m_correct(false)
    , m_started(false)
{
    m_segment = new lms_hls_segment();

    m_jitter = new lms_timestamp();
    m_jitter->set_correct_type(LmsTimeStamp::middle);

    m_elapsed = new DElapsedTimer();
}

lms_hls_product::~lms_hls_product()
{
    DFree(m_muxer);
    DFree(m_segment);
    DFree(m_jitter);
    DFree(m_elapsed);
}

void lms_hls_product::start()
{
    if (m_started) {
        return;
    }

    lms_server_config_struct *config = lms_config::instance()->get_server(m_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config->get_hls_enable(m_req)) {
        return;
    }

    m_correct = config->get_hls_time_jitter(m_req);
    int jitter_type = config->get_hls_time_jitter_type(m_req);
    m_jitter->set_correct_type(jitter_type);

    m_acodec = config->get_hls_acodec(m_req);
    m_vcodec = config->get_hls_vcodec(m_req);

    m_time_expired = config->get_hls_time_expired(m_req);

    bool is_265 = (m_vcodec == "h265") ? true : false;
    bool is_mp3 = (m_acodec == "mp3") ? true : false;
    m_has_video = (m_vcodec == "vn") ? false : true;
    m_has_audio = (m_acodec == "an") ? false : true;

    m_muxer = GetCodecTsMuxer();
    m_muxer->setTsHandler(TS_MUXER_CALLBACK(&lms_hls_product::onWriteFrame));
    m_muxer->initialize(is_265, is_mp3, m_has_video, m_has_audio);


    m_segment->start();

    m_elapsed->stop();
    m_started = true;
}

void lms_hls_product::stop()
{
    m_segment->stop();
    DFree(m_muxer);
    m_started = false;
    m_elapsed->start();
}

void lms_hls_product::reload()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config || !config->get_hls_enable(m_req)) {
        stop();
        return;
    }

    if (!m_started) {
        start();
        return;
    }

    m_time_expired = config->get_hls_time_expired(m_req);

    DString acodec = config->get_hls_acodec(m_req);
    DString vcodec = config->get_hls_vcodec(m_req);

    if ((acodec != m_acodec) || (vcodec != m_vcodec)) {
        m_vcodec = vcodec;
        m_acodec = acodec;

        if (m_segment->reload(true) != ERROR_SUCCESS) {
            stop();
            return;
        }

        bool is_265 = (m_vcodec == "h265") ? true : false;
        bool is_mp3 = (m_acodec == "mp3") ? true : false;
        m_has_video = (m_vcodec == "vn") ? false : true;
        m_has_audio = (m_acodec == "an") ? false : true;

        DFree(m_muxer);
        m_muxer = GetCodecTsMuxer();
        m_muxer->setTsHandler(TS_MUXER_CALLBACK(&lms_hls_product::onWriteFrame));
        m_muxer->initialize(is_265, is_mp3, m_has_video, m_has_audio);
    } else {
        m_segment->reload(false);
    }
}

void lms_hls_product::reset()
{
    m_has_video = true;
    m_has_audio = true;
    m_correct = false;
    m_started = false;

    m_segment->reset();
    m_jitter->reset();
}

bool lms_hls_product::timeExpired()
{
    return m_elapsed->hasExpired(m_time_expired * 1000 * 1000);
}

void lms_hls_product::setRequest(kernel_request *req)
{
    m_req = req;
    m_segment->setRequest(m_req);
}

int lms_hls_product::onVideo(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if (!m_started || !m_has_video) {
        return ret;
    }

    if (msg->is_sequence_header()) {
        if (m_muxer->setVideoSequenceHeader((uint8_t*)msg->payload->data + 5, msg->payload_length - 5) != ERROR_SUCCESS) {
            ret = ERROR_TS_MUXER_INIT_VIDEO;
        }
        m_segment->update_sequence();
        return ret;
    }

    char *buf = msg->payload->data;
    int size = msg->payload_length;

    bool keyframe = kernel_codec::video_is_keyframe(buf, size);

    dint64 dts = m_jitter->correct(msg);
    dint64 pts = msg->cts + dts;
    if (!m_correct) {
        dts = msg->dts;
        pts = msg->pts();
    }

    bool force_pat_pmt;
    if ((ret = m_segment->reap_segment(msg->dts, keyframe, force_pat_pmt)) != ERROR_SUCCESS) {
        return ret;
    }

    if (kernel_codec::video_is_h264(buf, size)) {
        ret = m_muxer->onVideo((uint8_t*)buf + 5, size - 5, dts, pts, true, force_pat_pmt);
    } else {
        ret = m_muxer->onVideo((uint8_t*)buf + 1, size - 1, dts, pts, false, force_pat_pmt);
    }

    if (ret != ERROR_SUCCESS) {
        ret = ERROR_TS_MUXER_VIDEO_MUXER;
    }

    return ret;
}

int lms_hls_product::onAudio(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if (!m_started || !m_has_audio) {
        return ret;
    }

    if (msg->is_sequence_header()) {
        if (m_muxer->setAudioSequenceHeader((uint8_t*)msg->payload->data + 2, msg->payload_length - 2) != ERROR_SUCCESS) {
            ret = ERROR_TS_MUXER_INIT_AUDIO;
        }
        m_segment->update_sequence();
        return ret;
    }

    char *buf = msg->payload->data;
    int size = msg->payload_length;

    dint64 dts = m_jitter->correct(msg);
    if (!m_correct) {
        dts = msg->dts;
    }

    bool force_pat_pmt;
    if ((ret = m_segment->reap_segment(msg->dts, false, force_pat_pmt)) != ERROR_SUCCESS) {
        return ret;
    }

    if (kernel_codec::audio_is_aac(buf, size)) {
        ret = m_muxer->onAudio((uint8_t*)buf + 2, size - 2, dts, true, force_pat_pmt);
    } else {
        ret = m_muxer->onAudio((uint8_t*)buf + 1, size - 1, dts, false, force_pat_pmt);
    }

    if (ret != ERROR_SUCCESS) {
        ret = ERROR_TS_MUXER_AUDIO_MUXER;
    }

    return ret;
}

int lms_hls_product::onMetadata(CommonMessage *msg)
{
    return ERROR_SUCCESS;
}

int lms_hls_product::onWriteFrame(unsigned char *p, unsigned int size)
{
    if (m_segment) {
        return m_segment->write_ts_data(p, size);
    }

    return ERROR_SUCCESS;
}

/*******************************************************/

lms_hls_segment::lms_hls_segment()
    : m_req(NULL)
    , m_ts_number(0)
    , m_temp_file(NULL)
    , m_current_pts(0)
    , m_start_pts(0)
    , m_duration(0)
    , m_has_video(true)
    , m_m3u8_number(0)
    , m_update_sequence(false)
    , m_started(false)
{

}

lms_hls_segment::~lms_hls_segment()
{
    DFree(m_temp_file);
}

int lms_hls_segment::start()
{
    int ret = ERROR_SUCCESS;

    lms_server_config_struct *config = lms_config::instance()->get_server(m_req);
    DAutoFree(lms_server_config_struct, config);

    m_root = config->get_hls_root(m_req);

    DString m3u8_path = config->get_hls_m3u8_path(m_req);
    m3u8_path.replace("[stream]", m_req->stream, true);

    m_ts_path = config->get_hls_ts_path(m_req);
    m_window = config->get_hls_window(m_req);
    m_fragment = config->get_hls_fragment(m_req) * 1000;

    m_m3u8_filename = m_root + "/" + m_req->vhost + "/" + m_req->app + "/" + m3u8_path;
    m_m3u8_filename.replace("//", "/", true);

    DString vcodec = config->get_hls_vcodec(m_req);
    m_has_video = (vcodec == "vn") ? false : true;

    m_temp_filename = m_root + "/" + m_req->vhost + "/" + m_req->app + "/" + m_req->stream + "/" + m_req->stream + ".tempts";

    if ((ret = reset_temp_file()) != ERROR_SUCCESS) {
        return ret;
    }

    m_started = true;

    return ret;
}

void lms_hls_segment::stop()
{
    if (!m_started) {
        return;
    }

    if (rename_ts() == ERROR_SUCCESS) {
        add_ts_to_m3u8();
    }

    m_update_sequence = false;
    m_started = false;
}

int lms_hls_segment::reload(bool force)
{
    int ret = ERROR_SUCCESS;

    lms_server_config_struct *config = lms_config::instance()->get_server(m_req);
    DAutoFree(lms_server_config_struct, config);

    m_root = config->get_hls_root(m_req);

    DString m3u8_path = config->get_hls_m3u8_path(m_req);
    m3u8_path.replace("[stream]", m_req->stream, true);

    m_ts_path = config->get_hls_ts_path(m_req);
    m_window = config->get_hls_window(m_req);
    m_fragment = config->get_hls_fragment(m_req) * 1000;

    m_m3u8_filename = m_root + "/" + m_req->vhost + "/" + m_req->app + "/" + m3u8_path;
    m_m3u8_filename.replace("//", "/", true);

    DString vcodec = config->get_hls_vcodec(m_req);
    m_has_video = (vcodec == "vn") ? false : true;

    m_temp_filename = m_root + "/" + m_req->vhost + "/" + m_req->app + "/" + m_req->stream + "/" + m_req->stream + ".tempts";

    if (force && (ret = update_segment()) != ERROR_SUCCESS) {
        return ret;
    }

    return ret;
}

void lms_hls_segment::reset()
{
    m_ts_number = 0;
    m_current_pts = 0;
    m_start_pts = 0;
    m_duration = 0;
    m_has_video = true;
    m_m3u8_number = 0;
    m_update_sequence = false;

    m_ts_list.clear();
}

int lms_hls_segment::reap_segment(int64_t pts, bool keyframe, bool &force_pat_pmt)
{
    int ret = ERROR_SUCCESS;

    m_current_pts = pts;
    if (m_start_pts == 0) {
        m_start_pts = pts;
    }

    m_duration = m_current_pts - m_start_pts;
    if (m_duration < m_fragment) {
        force_pat_pmt = false;
        return ret;
    }

    if (m_has_video && !keyframe) {
        if (m_duration < m_fragment + 10000) {
            force_pat_pmt = false;
            return ret;
        }
    }

    if ((ret = update_segment()) != ERROR_SUCCESS) {
        log_error("hls update segment failed. ret=%d", ret);
        return ret;
    }

    m_start_pts = m_current_pts;
    force_pat_pmt = true;
    m_update_sequence = false;

    return ret;
}

void lms_hls_segment::update_sequence()
{
    m_update_sequence = true;
}

void lms_hls_segment::setRequest(kernel_request *req)
{
    m_req = req;
}

int lms_hls_segment::write_ts_data(unsigned char *p, unsigned int size)
{
    int ret = ERROR_SUCCESS;

    if (m_temp_file) {
        if (m_temp_file->write((const char*)p, size) != size) {
            ret = ERROR_WRITE_FILE;
            log_error("write ts data to temp file failed. ret=%d", ret);
            return ret;
        }
    }
    return ret;
}

DString lms_hls_segment::generate_ts_filename()
{
    DString ts_filename = m_ts_path;

    struct timeval tv;
    gettimeofday(&tv, NULL);

    struct tm *tm;
    if ((tm = localtime(&tv.tv_sec)) == NULL) {
        return DString::number(m_ts_number) + ".ts";
    }

    DString yyyy = DString().sprintf("%d",   1900 + tm->tm_year);
    DString MM   = DString().sprintf("%02d", 1 + tm->tm_mon);
    DString dd   = DString().sprintf("%02d", tm->tm_mday);
    DString hh   = DString().sprintf("%02d", tm->tm_hour);
    DString mm   = DString().sprintf("%02d", tm->tm_min);
    DString ss   = DString().sprintf("%02d", tm->tm_sec);

    ts_filename.replace("[yyyy]", yyyy, true);
    ts_filename.replace("[MM]", MM, true);
    ts_filename.replace("[dd]", dd, true);
    ts_filename.replace("[hh]", hh, true);
    ts_filename.replace("[mm]", mm, true);
    ts_filename.replace("[ss]", ss, true);

    ts_filename.replace("[vhost]", m_req->vhost, true);
    ts_filename.replace("[app]", m_req->app, true);
    ts_filename.replace("[stream]", m_req->stream, true);

    ts_filename.replace("[num]",DString::number(m_ts_number++), true);

    if (!ts_filename.endWith(".ts")) {
        ts_filename.append(".ts");
    }

    return ts_filename;
}

bool lms_hls_segment::create_dir(const DString &dir)
{
    if (!DDir::exists(dir)) {
        if (!DDir::createDir(dir)) {
            return false;
        }
    }
    return true;
}

int lms_hls_segment::update_segment()
{
    int ret = ERROR_SUCCESS;

    if ((ret = rename_ts()) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = reset_temp_file()) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = add_ts_to_m3u8()) != ERROR_SUCCESS) {
        return ret;
    }

    return ret;
}

int lms_hls_segment::reset_temp_file()
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

    return ret;
}

int lms_hls_segment::rename_ts()
{
    int ret = ERROR_SUCCESS;

    DFree(m_temp_file);

    m_ts_filename = m_root + "/" + m_req->vhost + "/" + m_req->app + "/" + m_req->stream + "/" + generate_ts_filename();

    if (::rename(m_temp_filename.c_str(), m_ts_filename.c_str()) < 0) {
        ret = ERROR_RENAME_FILE;
        log_error("rename ts file failed. %s => %s, ret=%d", m_temp_filename.c_str(), m_ts_filename.c_str(), ret);
        return ret;
    }

    return ret;
}

int lms_hls_segment::add_ts_to_m3u8()
{
    DString m3u8_ts_filename = m_ts_filename;

    DString m3u8_path = DFile::filePath(m_m3u8_filename);
    if (!m3u8_path.endWith("/")) {
        m3u8_path.append("/");
    }
    m3u8_path.replace("//", "/", true);

    m3u8_ts_filename = m3u8_ts_filename.split(m3u8_path).at(0);

    TsInfo info;
    info.duration = (double)m_duration / 1000;
    info.filename = m3u8_ts_filename;
    info.update_sequence = m_update_sequence;
    info.number = m_m3u8_number++;

    m_ts_list.push_back(info);

    return build_m3u8();
}

int lms_hls_segment::build_m3u8()
{
    int ret = ERROR_SUCCESS;

    if (m_ts_list.size() > m_window) {
        m_ts_list.pop_front();
    }

    DString data;

    data << "#EXTM3U\n";
    data << "#EXT-X-VERSION:3\n";
    data << "#EXT-X-ALLOW-CACHE:YES\n";

    TsInfo info = *m_ts_list.begin();
    data << "#EXT-X-MEDIA-SEQUENCE:" << info.number << "\n";

    list<TsInfo>::iterator it;
    int target_duration = 0;
    for (it = m_ts_list.begin(); it != m_ts_list.end(); ++it) {
        TsInfo temp = *it;
        target_duration = DMax(target_duration, (int)ceil(temp.duration));
    }
    data << "#EXT-X-TARGETDURATION:" << target_duration << "\n";

    for (it = m_ts_list.begin(); it != m_ts_list.end(); ++it) {
        TsInfo temp = *it;

        if (temp.update_sequence) {
            data << "#EXT-X-DISCONTINUITY\n";
        }

        data << "#EXTINF:" << DString::number(temp.duration) << ", no desc" << "\n";
        data << temp.filename << "\n";
    }

    DString dir = DFile::filePath(m_m3u8_filename);
    if (!create_dir(dir)) {
        ret = ERROR_CREATE_DIR;
        log_error("create dir %s failed. ret=%d", dir.c_str(), ret);
        return ret;
    }

    DString m3u8_filename_temp = m_m3u8_filename + ".temp";
    DFile file(m3u8_filename_temp);
    if (!file.open("w")) {
        ret = ERROR_OPEN_FILE;
        log_error("open m3u8 temp file failed. filename=%s, ret=%d", m3u8_filename_temp.c_str(), ret);
        return ret;
    }

    if (file.write(data) != data.size()) {
        ret = ERROR_WRITE_FILE;
        log_error("write m3u8 data to file failed. ret=%d", ret);
        return ret;
    }

    file.close();

    if (::rename(m3u8_filename_temp.c_str(), m_m3u8_filename.c_str()) < 0) {
        ret = ERROR_RENAME_FILE;
        log_error("rename m3u8 filename failed. filename=%s, ret=%d", m_m3u8_filename.c_str(), ret);
        return ret;
    }

    return ret;
}
