#include "lms_http_ts_live.hpp"
#include "lms_http_server_conn.hpp"
#include "lms_http_utility.hpp"
#include "kernel_log.hpp"
#include "kernel_errno.hpp"
#include "kernel_codec.hpp"
#include "lms_config.hpp"
#include "DHttpHeader.hpp"
#include "lms_global.hpp"

lms_http_ts_live::lms_http_ts_live(lms_http_server_conn *conn)
    : m_conn(conn)
    , m_writer(NULL)
    , m_req(NULL)
    , m_source(NULL)
    , m_muxer(NULL)
    , m_enable(false)
    , m_chunked(true)
    , m_player_buffer_length(1000)
    , m_is_edge(false)
    , m_timeout(10 * 1000 * 1000)
    , m_has_video(true)
    , m_has_audio(true)
{

}

lms_http_ts_live::~lms_http_ts_live()
{
    DFree(m_writer);
    DFree(m_req);
    DFree(m_muxer);
}

int lms_http_ts_live::initialize(DHttpParser *parser)
{
    int ret = ERROR_SUCCESS;

    m_req = get_http_request(parser);
    if (m_req == NULL) {
        ret = ERROR_HTTP_GENERATE_REQUEST;
        log_error("generate http request failed. ret=%d", ret);
        return ret;
    }

    get_config_value();

    if (!m_enable) {
        ret = ERROR_HTTP_TS_LIVE_REJECT;
        return ret;
    }

    m_source = lms_source_manager::instance()->addSource(m_req);

    if (!m_source->add_connection(m_conn)) {
        ret = ERROR_SOURCE_ADD_CONNECTION;
        return ret;
    }

    if (!m_source->add_reload_conn(m_conn)) {
        ret = ERROR_SOURCE_ADD_RELOAD;
        return ret;
    }

    if (!m_source->onPlay(m_conn->getEvent(), m_is_edge)) {
        ret = ERROR_SOURCE_ONPLAY;
        return ret;
    }

    init_muxer();

    m_writer = new lms_stream_writer(m_req, AV_Handler_Callback(&lms_http_ts_live::onSendMessage), false);

    return ret;
}

int lms_http_ts_live::start()
{
    int ret = ERROR_SUCCESS;

    m_conn->setReadTimeOut(-1);
    m_conn->setWriteTimeOut(m_timeout);

    if ((ret = response_http_header()) != ERROR_SUCCESS) {
        if (ret != SOCKET_EAGAIN) {
            return ret;
        }
    }

    if ((ret = m_source->send_gop_cache(m_writer, m_player_buffer_length)) != ERROR_SUCCESS) {
        if (ret != SOCKET_EAGAIN) {
            return ret;
        }
    }

    return ERROR_SUCCESS;
}

int lms_http_ts_live::flush()
{
    if (m_writer) {
        return m_writer->flush();
    }

    return ERROR_SUCCESS;
}

int lms_http_ts_live::process(CommonMessage *msg)
{
    if (m_writer) {
        return m_writer->send(msg);
    }

    return ERROR_SUCCESS;
}

bool lms_http_ts_live::reload()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        return false;
    }

    m_enable = config->get_ts_live_enable(m_req);
    if (!m_enable) {
        return false;
    }

    int timeout = config->get_http_timeout(m_req);
    m_timeout = timeout * 1000 * 1000;
    m_conn->setWriteTimeOut(m_timeout);

    if (m_writer) {
        m_writer->reload();
    }

    return true;
}

void lms_http_ts_live::release()
{
    if (m_muxer) {
        m_muxer->close();
    }

    if (m_source) {
        m_source->del_connection(m_conn);
        m_source->del_reload_conn(m_conn);
    }
}

void lms_http_ts_live::get_config_value()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        return;
    }

    m_enable = config->get_ts_live_enable(m_req);
    m_acodec = config->get_ts_live_acodec(m_req);
    m_vcodec = config->get_ts_live_vcodec(m_req);

    m_chunked = config->get_http_chunked(m_req);
    m_player_buffer_length = config->get_http_buffer_length(m_req);
    m_is_edge = config->get_proxy_enable(m_req);

    int timeout = config->get_http_timeout(m_req);
    m_timeout = timeout * 1000 * 1000;
}

int lms_http_ts_live::response_http_header()
{
    int ret = ERROR_SUCCESS;

    DHttpHeader header;
    header.setServer(LMS_VERSION);

    header.setContentType("ts");
    header.setConnectionKeepAlive();

    if (m_chunked) {
        header.addValue("Transfer-Encoding", "chunked");
    }

    DString str = header.getResponseString(200);

    MemoryChunk *chunk = DMemPool::instance()->getMemory(str.size());
    DSharedPtr<MemoryChunk> response = DSharedPtr<MemoryChunk>(chunk);

    memcpy(response->data, str.data(), str.size());
    response->length = str.size();

    if ((ret = m_conn->write(response, str.size())) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_HTTP_SEND_RESPONSE_HEADER, "ts live write http header failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int lms_http_ts_live::onSendMessage(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if (msg->is_metadata()) {
        return ret;
    }

    if (!m_has_video && msg->is_video()) {
        return ret;
    }

    if (!m_has_audio && msg->is_audio()) {
        return ret;
    }

    if (msg->is_sequence_header()) {
        if (msg->is_video()) {
            if (m_muxer->setVideoSequenceHeader((uint8_t*)msg->payload->data + 5, msg->payload_length - 5) != ERROR_SUCCESS) {
                ret = ERROR_TS_MUXER_INIT_VIDEO;
                return ret;
            }
        } else if (msg->is_audio()) {
            if (m_muxer->setAudioSequenceHeader((uint8_t*)msg->payload->data + 2, msg->payload_length - 2) != ERROR_SUCCESS) {
                ret = ERROR_TS_MUXER_INIT_AUDIO;
                return ret;
            }
        }

        return ret;
    }

    char *buf = msg->payload->data;
    int size = msg->payload_length;

    if (msg->is_video()) {
        bool force_pat_pmt = kernel_codec::video_is_keyframe(buf, size);

        if (kernel_codec::video_is_h264(buf, size)) {
            ret = m_muxer->onVideo((uint8_t*)buf + 5, size - 5, msg->dts, msg->pts(), true, force_pat_pmt);
        } else {
            ret = m_muxer->onVideo((uint8_t*)buf + 1, size - 1, msg->dts, msg->pts(), false, force_pat_pmt);
        }

        if (ret != ERROR_SUCCESS) {
            ret = ERROR_TS_MUXER_VIDEO_MUXER;
        }

    } else if (msg->is_audio()) {
        if (kernel_codec::audio_is_aac(buf, size)) {
            ret = m_muxer->onAudio((uint8_t*)buf + 2, size - 2, msg->dts, true, false);
        } else {
            ret = m_muxer->onAudio((uint8_t*)buf + 1, size - 1, msg->dts, false, false);
        }

        if (ret != ERROR_SUCCESS) {
            ret = ERROR_TS_MUXER_AUDIO_MUXER;
        }
    }

    if ((ret = m_conn->flush()) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_HTTP_WRITE_TS_DATA, "write http ts data failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int lms_http_ts_live::onWriteFrame(unsigned char *p, unsigned int size)
{
    int ret = ERROR_SUCCESS;

    if (m_chunked) {
        // 64 => chunk size
        // 2  => \r\n
        MemoryChunk *chunk = DMemPool::instance()->getMemory(64 + 2);
        DSharedPtr<MemoryChunk> begin = DSharedPtr<MemoryChunk>(chunk);

        int chunked_size = size;
        int nb_size = snprintf(begin->data, 64, "%x", chunked_size);

        begin->length = nb_size + 2;

        char *p = begin->data + nb_size;
        *p++ = '\r';
        *p++ = '\n';

        m_conn->add(begin, begin->length);
    }

    MemoryChunk *ch = DMemPool::instance()->getMemory(size);
    DSharedPtr<MemoryChunk> body = DSharedPtr<MemoryChunk>(ch);
    memcpy(body->data, p, size);
    body->length = size;
    m_conn->add(body, size);

    if (m_chunked) {
        // 2 => \r\n
        MemoryChunk *chunk = DMemPool::instance()->getMemory(2);
        DSharedPtr<MemoryChunk> end = DSharedPtr<MemoryChunk>(chunk);

        end->length = 2;

        char *p = end->data;
        *p++ = '\r';
        *p++ = '\n';

        m_conn->add(end, end->length);
    }

    return ret;
}

void lms_http_ts_live::init_muxer()
{
    if (m_muxer) {
        m_muxer->close();
        DFree(m_muxer);
    }

    bool is_265 = (m_vcodec == "h265") ? true : false;
    bool is_mp3 = (m_acodec == "mp3") ? true : false;
    m_has_video = (m_vcodec == "vn") ? false : true;
    m_has_audio = (m_acodec == "an") ? false : true;

    m_muxer = GetCodecTsMuxer();
    m_muxer->setTsHandler(TS_MUXER_CALLBACK(&lms_http_ts_live::onWriteFrame));
    m_muxer->initialize(is_265, is_mp3, m_has_video, m_has_audio);
}
