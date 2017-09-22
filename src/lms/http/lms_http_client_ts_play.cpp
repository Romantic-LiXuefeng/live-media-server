#include "lms_http_client_ts_play.hpp"
#include "lms_edge.hpp"
#include "DHostInfo.hpp"
#include "kernel_request.hpp"
#include "kernel_log.hpp"
#include "kernel_errno.hpp"
#include "lms_config.hpp"
#include "lms_source.hpp"
#include "DHttpHeader.hpp"

lms_http_client_ts_play::lms_http_client_ts_play(lms_edge *parent, DEvent *event, lms_source *source)
    : DTcpSocket(event)
    , m_parent(parent)
    , m_source(source)
    , m_src_req(NULL)
    , m_dst_req(NULL)
    , m_demuxer(NULL)
    , m_timeout(30 * 1000 * 1000)
    , m_started(false)
    , m_pkt_size(188)
{
    m_reader = new http_reader(this, false, HTTP_HEADER_CALLBACK(&lms_http_client_ts_play::onHttpParser));

    setWriteTimeOut(m_timeout);
    setReadTimeOut(m_timeout);
}

lms_http_client_ts_play::~lms_http_client_ts_play()
{
    DFree(m_reader);
    DFree(m_demuxer);
}

void lms_http_client_ts_play::start(kernel_request *src, kernel_request *dst, const DString &host, int port)
{
    m_src_req = src;
    m_dst_req = dst;
    m_port = port;

    get_config_value();

    log_info("ts play client start. host=%s, port=%d", host.c_str(), port);

    DHostInfo *h = new DHostInfo(m_event);
    h->setFinishedHandler(HOST_CALLBACK(&lms_http_client_ts_play::onHost));
    h->setErrorHandler(HOST_CALLBACK(&lms_http_client_ts_play::onHostError));

    if (!h->lookupHost(host)) {
        log_error("lookup host failed. ret=%d", ERROR_LOOKUP_HOST);
        h->close();
        release();
    }
}

void lms_http_client_ts_play::release()
{
    global_context->update_id(m_fd);

    log_trace("ts play client released");

    m_source->stop_external();

    m_parent->release();

    global_context->delete_id(m_fd);

    close();
}

void lms_http_client_ts_play::reload()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_src_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        release();
        return;
    }

    int timeout = config->get_http_timeout(m_src_req);
    m_timeout = timeout * 1000 * 1000;

    setReadTimeOut(m_timeout);
}

int lms_http_client_ts_play::onReadProcess()
{
    int ret = ERROR_SUCCESS;

    global_context->update_id(m_fd);

    if ((ret = m_reader->service()) != ERROR_SUCCESS) {
        if (ret != SOCKET_EAGAIN) {
            log_error("ts play client read and parse http request failed. ret=%d", ret);
            return ret;
        }
    }

    while (!m_reader->empty()) {
        MemoryChunk *chunk = DMemPool::instance()->getMemory(m_pkt_size);
        DSharedPtr<MemoryChunk> payload = DSharedPtr<MemoryChunk>(chunk);
        payload->length = m_pkt_size;

        if ((ret = m_reader->readBody(payload->data, m_pkt_size)) != ERROR_SUCCESS) {
            log_error_eagain(ret, ERROR_TS_READ_BODY, "http client read ts body failed. ret=%d", ret);
            return ret;
        }

        if (m_demuxer->demuxer(payload->data) != ERROR_SUCCESS) {
            ret = ERROR_TS_DEMUXER;
            return ret;
        }
    }

    return ret;
}

int lms_http_client_ts_play::onWriteProcess()
{
    int ret = ERROR_SUCCESS;

    global_context->update_id(m_fd);

    if ((ret = do_start()) != ERROR_SUCCESS) {
        return ret;
    }

    return ret;
}

void lms_http_client_ts_play::onReadTimeOutProcess()
{
    release();
}

void lms_http_client_ts_play::onWriteTimeOutProcess()
{
    release();
}

void lms_http_client_ts_play::onErrorProcess()
{
    release();
}

void lms_http_client_ts_play::onCloseProcess()
{
    release();
}

void lms_http_client_ts_play::get_config_value()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_src_req);
    DAutoFree(lms_server_config_struct, config);

    int timeout = config->get_http_timeout(m_src_req);
    m_timeout = timeout * 1000 * 1000;
}

void lms_http_client_ts_play::onHost(const DStringList &ips)
{
    int ret = ERROR_SUCCESS;

    if (ips.empty()) {
        release();
        return;
    }

    DString ip = ips.at(0);
    log_info("ts play client lookup origin ip=[%s]", ip.c_str());

    if ((ret = connectToHost(ip.c_str(), m_port)) != ERROR_SUCCESS) {
        if (ret != SOCKET_EINPROGRESS) {
            ret = ERROR_TCP_SOCKET_CONNECT;
            log_error("ts play client connect to host failed. ip=%s, port=%d, ret=%d", ip.c_str(), m_port, ret);
            release();
        }
    }
}

void lms_http_client_ts_play::onHostError(const DStringList &ips)
{
    log_error("ts play client get host failed.");
    release();
}

int lms_http_client_ts_play::onHttpParser(DHttpParser *parser)
{
    int ret = ERROR_SUCCESS;

    duint16 status_code = parser->statusCode();

    if (status_code != 200) {
        ret = ERROR_HTTP_TS_PULL_REJECTED;
        log_error("http response is code is rejected, status_code=%d. ret=%d", status_code, ret);
        return ret;
    }

    setWriteTimeOut(-1);
    setReadTimeOut(m_timeout);

    m_source->start_external();

    m_demuxer = GetCodecTsDemuxer();
    m_demuxer->initialize(m_pkt_size);
    m_demuxer->setHandler(TS_DEMUXER_CALLBACK(&lms_http_client_ts_play::onRecvMessage));

    return ret;
}

int lms_http_client_ts_play::send_http_header()
{
    int ret = ERROR_SUCCESS;

    DHttpHeader header;

    header.setConnectionKeepAlive();
    DString host = m_dst_req->vhost + ":" + DString::number(m_port);
    header.setHost(host);
    header.addValue("Accept", "*/*");

    DString uri = "/" + m_dst_req->app + "/" + m_dst_req->stream + ".ts?type=live";
    DString str = header.getRequestString("GET", uri);

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

int lms_http_client_ts_play::do_start()
{
    if (m_started) {
        return ERROR_SUCCESS;
    }

    global_context->set_id(m_fd);
    global_context->update_id(m_fd);

    m_started = true;

    log_trace("ts play client start");

    return send_http_header();
}

int lms_http_client_ts_play::onRecvMessage(TsDemuxerPacket pkt)
{
    int ret = ERROR_SUCCESS;

    if (pkt.type == LibCodecStreamType::Audio) {
        if (pkt.codec == LibCodecAVType::MP3) {
            ret = create_mp3_message(pkt);
        } else if (pkt.codec == LibCodecAVType::AAC) {
            ret = create_aac_message(pkt);
        }
    } else if (pkt.type == LibCodecStreamType::Video) {
        if (pkt.codec == LibCodecAVType::AVC) {
            ret = create_avc_message(pkt);
        } else if (pkt.codec == LibCodecAVType::HEVC) {
            ret = create_hevc_message(pkt);
        }
    }

    return ret;
}

int lms_http_client_ts_play::create_aac_message(TsDemuxerPacket &pkt)
{
    MemoryChunk *chunk = DMemPool::instance()->getMemory(pkt.len + 2);
    DSharedPtr<MemoryChunk> payload = DSharedPtr<MemoryChunk>(chunk);
    payload->length = pkt.len + 2;

    char *p = payload->data;
    p[0] = 0xaf;

    if (pkt.sequence_header) {
        p[1] = 0x00;
    } else {
        p[1] = 0x01;
    }

    memcpy(payload->data + 2, (char*)pkt.buf, pkt.len);

    CommonMessage msg;
    msg.dts = pkt.dts;
    msg.cts = pkt.pts - pkt.dts;
    msg.payload = payload;
    msg.payload_length = pkt.len + 2;

    if (pkt.sequence_header) {
        msg.sequence_header = true;
    } else {
        msg.sequence_header = false;
    }

    msg.keyframe = false;
    msg.type = CommonMessageAudio;

    return process_message(&msg);
}

int lms_http_client_ts_play::create_mp3_message(TsDemuxerPacket &pkt)
{
    MemoryChunk *chunk = DMemPool::instance()->getMemory(pkt.len + 1);
    DSharedPtr<MemoryChunk> payload = DSharedPtr<MemoryChunk>(chunk);
    payload->length = pkt.len + 1;

    char *p = payload->data;
    p[0] = 0x2f;
    memcpy(payload->data + 1, (char*)pkt.buf, pkt.len);

    CommonMessage msg;
    msg.dts = pkt.dts;
    msg.cts = pkt.pts - pkt.dts;
    msg.payload = payload;
    msg.payload_length = pkt.len + 1;
    msg.sequence_header = false;
    msg.keyframe = false;
    msg.type = CommonMessageAudio;

    return process_message(&msg);
}

int lms_http_client_ts_play::create_avc_message(TsDemuxerPacket &pkt)
{
    MemoryChunk *chunk = DMemPool::instance()->getMemory(pkt.len + 5);
    DSharedPtr<MemoryChunk> payload = DSharedPtr<MemoryChunk>(chunk);
    payload->length = pkt.len + 5;

    char *p = payload->data;
    if (pkt.keyframe) {
        p[0] = 0x17;
    } else {
        p[0] = 0x27;
    }

    if (pkt.sequence_header) {
        p[1] = 0x00;
    } else {
        p[1] = 0x01;
    }

    int cts = pkt.pts - pkt.dts;
    char *pp = (char*)&cts;

    p[2] = pp[2];
    p[3] = pp[1];
    p[4] = pp[0];

    memcpy(payload->data + 5, (char*)pkt.buf, pkt.len);

    CommonMessage msg;
    msg.dts = pkt.dts;
    msg.cts = pkt.pts - pkt.dts;
    msg.payload = payload;
    msg.payload_length = pkt.len + 5;

    if (pkt.sequence_header) {
        msg.sequence_header = true;
    } else {
        msg.sequence_header = false;
    }

    if (pkt.keyframe) {
        msg.keyframe = true;
    } else {
        msg.keyframe = false;
    }

    msg.type = CommonMessageVideo;

    return process_message(&msg);
}

int lms_http_client_ts_play::create_hevc_message(TsDemuxerPacket &pkt)
{
    MemoryChunk *chunk = DMemPool::instance()->getMemory(pkt.len + 1);
    DSharedPtr<MemoryChunk> payload = DSharedPtr<MemoryChunk>(chunk);
    payload->length = pkt.len + 1;

    char *p = payload->data;

    if (pkt.keyframe) {
        p[0] = 0x1a;
    } else {
        p[0] = 0x2a;
    }

    memcpy(payload->data + 1, (char*)pkt.buf, pkt.len);

    CommonMessage msg;
    msg.dts = pkt.dts;
    msg.cts = pkt.pts - pkt.dts;
    msg.payload = payload;
    msg.payload_length = pkt.len + 1;
    msg.sequence_header = false;
    if (pkt.keyframe) {
        msg.keyframe = true;
    } else {
        msg.keyframe = false;
    }
    msg.type = CommonMessageVideo;

    return process_message(&msg);
}

int lms_http_client_ts_play::process_message(CommonMessage *msg)
{
    if (msg->is_audio()) {
        return m_source->onAudio(msg);
    } else if (msg->is_video()) {
        return m_source->onVideo(msg);
    } else if (msg->is_metadata()) {
        return m_source->onMetadata(msg);
    }

    return ERROR_SUCCESS;
}
