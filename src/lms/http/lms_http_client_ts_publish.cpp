#include "lms_http_client_ts_publish.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"
#include "kernel_codec.hpp"
#include "lms_config.hpp"
#include "lms_edge.hpp"
#include "DHostInfo.hpp"
#include "lms_source.hpp"
#include "DHttpHeader.hpp"
#include "lms_global.hpp"

lms_http_client_ts_publish::lms_http_client_ts_publish(lms_edge *parent, DEvent *event, lms_gop_cache *gop_cache)
    : DTcpSocket(event)
    , m_parent(parent)
    , m_gop_cache(gop_cache)
    , m_src_req(NULL)
    , m_dst_req(NULL)
    , m_writer(NULL)
    , m_muxer(NULL)
    , m_timeout(30 * 1000 * 1000)
    , m_started(false)
    , m_chunked(true)
    , m_has_video(true)
    , m_has_audio(true)
{
    setWriteTimeOut(m_timeout);
    setReadTimeOut(m_timeout);

}

lms_http_client_ts_publish::~lms_http_client_ts_publish()
{
    DFree(m_writer);
    DFree(m_muxer);
}

void lms_http_client_ts_publish::start(kernel_request *src, kernel_request *dst, const DString &host, int port)
{
    m_src_req = src;
    m_dst_req = dst;
    m_port = port;

    get_config_value();

    log_info("ts publish client start. host=%s, port=%d", host.c_str(), port);

    DHostInfo *h = new DHostInfo(m_event);
    h->setFinishedHandler(HOST_CALLBACK(&lms_http_client_ts_publish::onHost));
    h->setErrorHandler(HOST_CALLBACK(&lms_http_client_ts_publish::onHostError));

    if (!h->lookupHost(host)) {
        log_error("ts publish client lookup host failed. ret=%d", ERROR_LOOKUP_HOST);
        h->close();
        release();
    }
}

void lms_http_client_ts_publish::release()
{
    global_context->update_id(m_fd);

    log_trace("ts publish client released");

    m_parent->release();

    global_context->delete_id(m_fd);

    close();
}

void lms_http_client_ts_publish::process(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if (m_writer && ((ret = m_writer->send(msg)) != ERROR_SUCCESS)) {
        if (ret != SOCKET_EAGAIN) {
            log_error("ts publish client send message failed. ret=%d", ret);
            release();
        }
    }
}

void lms_http_client_ts_publish::reload()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_src_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        release();
        return;
    }

    int timeout = config->get_http_timeout(m_src_req);
    m_timeout = timeout * 1000 * 1000;

    setReadTimeOut(-1);
    setWriteTimeOut(m_timeout);

    if (m_writer) {
        m_writer->reload();
    }
}

int lms_http_client_ts_publish::onReadProcess()
{
    int len = getReadBufferLength();
    if (len > 0) {
        MemoryChunk *chunk = DMemPool::instance()->getMemory(len);
        DSharedPtr<MemoryChunk> payload = DSharedPtr<MemoryChunk>(chunk);
        read(payload->data, len);
    }

    return ERROR_SUCCESS;
}

int lms_http_client_ts_publish::onWriteProcess()
{
    int ret = ERROR_SUCCESS;

    global_context->update_id(m_fd);

    if ((ret = do_start()) != ERROR_SUCCESS) {
        return ret;
    }

    if (m_writer && ((ret = m_writer->flush()) != ERROR_SUCCESS)) {
        return ret;
    }

    return ret;
}

void lms_http_client_ts_publish::onReadTimeOutProcess()
{
    log_error("read timeout");
    release();
}

void lms_http_client_ts_publish::onWriteTimeOutProcess()
{
    log_error("write timeout");
    release();
}

void lms_http_client_ts_publish::onErrorProcess()
{
    log_error("socket error");
    release();
}

void lms_http_client_ts_publish::onCloseProcess()
{
    log_error("socket closed");
    release();
}

void lms_http_client_ts_publish::get_config_value()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_src_req);
    DAutoFree(lms_server_config_struct, config);

    int timeout = config->get_http_timeout(m_src_req);
    m_timeout = timeout * 1000 * 1000;

    m_chunked = config->get_http_chunked(m_src_req);

    m_acodec = config->get_proxy_ts_acodec(m_src_req);
    m_vcodec = config->get_proxy_ts_vcodec(m_src_req);
}

void lms_http_client_ts_publish::onHost(const DStringList &ips)
{
    int ret = ERROR_SUCCESS;

    if (ips.empty()) {
        release();
        return;
    }

    DString ip = ips.at(0);
    log_info("ts publish client lookup origin ip=[%s]", ip.c_str());

    if ((ret = connectToHost(ip.c_str(), m_port)) != ERROR_SUCCESS) {
        if (ret != SOCKET_EINPROGRESS) {
            ret = ERROR_TCP_SOCKET_CONNECT;
            log_error("ts publish client connect to host failed. ip=%s, port=%d, ret=%d", ip.c_str(), m_port, ret);
            release();
        }
    }
}

void lms_http_client_ts_publish::onHostError(const DStringList &ips)
{
    log_error("ts publish client get host failed.");
    release();
}

int lms_http_client_ts_publish::do_start()
{
    int ret = ERROR_SUCCESS;

    if (m_started) {
        return ERROR_SUCCESS;
    }

    global_context->set_id(m_fd);
    global_context->update_id(m_fd);

    m_started = true;

    setReadTimeOut(-1);
    setWriteTimeOut(m_timeout);

    if ((ret = send_http_header()) != ERROR_SUCCESS) {
        return ret;
    }

    init_muxer();

    m_writer = new lms_stream_writer(m_src_req, AV_Handler_Callback(&lms_http_client_ts_publish::onSendMessage), true);

    if ((ret = m_writer->send_gop_messages(m_gop_cache, 0)) != ERROR_SUCCESS) {
        if (ret != SOCKET_EAGAIN) {
            return ret;
        }
    }

    return ret;
}

int lms_http_client_ts_publish::onSendMessage(CommonMessage *msg)
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

    if ((ret = flush()) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_HTTP_WRITE_TS_DATA, "write http ts data failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int lms_http_client_ts_publish::send_http_header()
{
    int ret = ERROR_SUCCESS;

    DHttpHeader header;
    if (m_chunked) {
        header.addValue("Transfer-Encoding", "chunked");
    }

    header.setConnectionKeepAlive();
    DString host = m_dst_req->vhost + ":" + DString::number(m_port);
    header.setHost(host);

    header.addValue("Accept", "*/*");
    header.addValue("Icy-MetaData", "1");

    DString uri = "/" + m_dst_req->app + "/" + m_dst_req->stream + ".ts";
    DString str = header.getRequestString("POST", uri);

    MemoryChunk *chunk = DMemPool::instance()->getMemory(str.size());
    DSharedPtr<MemoryChunk> h = DSharedPtr<MemoryChunk>(chunk);

    memcpy(h->data, str.data(), str.size());
    h->length = str.size();

    if ((ret = write(h, h->length)) != ERROR_SUCCESS) {
        if (ret != SOCKET_EAGAIN) {
            return ERROR_HTTP_SEND_REQUEST_HEADER;
        }
    }

    return ERROR_SUCCESS;
}

void lms_http_client_ts_publish::init_muxer()
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
    m_muxer->setTsHandler(TS_MUXER_CALLBACK(&lms_http_client_ts_publish::onWriteFrame));
    m_muxer->initialize(is_265, is_mp3, m_has_video, m_has_audio);
}

int lms_http_client_ts_publish::onWriteFrame(unsigned char *p, unsigned int size)
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

        add(begin, begin->length);
    }

    MemoryChunk *ch = DMemPool::instance()->getMemory(size);
    DSharedPtr<MemoryChunk> body = DSharedPtr<MemoryChunk>(ch);
    memcpy(body->data, p, size);
    body->length = size;
    add(body, size);

    if (m_chunked) {
        // 2 => \r\n
        MemoryChunk *chunk = DMemPool::instance()->getMemory(2);
        DSharedPtr<MemoryChunk> end = DSharedPtr<MemoryChunk>(chunk);

        end->length = 2;

        char *p = end->data;
        *p++ = '\r';
        *p++ = '\n';

        add(end, end->length);
    }

    return ret;
}
