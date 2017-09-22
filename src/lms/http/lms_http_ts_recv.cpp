#include "lms_http_ts_recv.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"
#include "lms_config.hpp"
#include "lms_http_server_conn.hpp"
#include "lms_http_utility.hpp"
#include "lms_global.hpp"
#include "DHttpHeader.hpp"

#define DEFAULT_TS_PACKET_SIZE      188

lms_http_ts_recv::lms_http_ts_recv(lms_http_server_conn *conn)
    : m_conn(conn)
    , m_req(NULL)
    , m_source(NULL)
    , m_demuxer(NULL)
    , m_enable(false)
    , m_timeout(10 * 1000 * 1000)
    , m_pkt_size(DEFAULT_TS_PACKET_SIZE)
{
    m_reader = m_conn->reader();
}

lms_http_ts_recv::~lms_http_ts_recv()
{
    log_warn("-----------> free lms_http_ts_recv");
    DFree(m_req);
    DFree(m_demuxer);
}

int lms_http_ts_recv::initialize(DHttpParser *parser)
{
    int ret = ERROR_SUCCESS;

    m_req = get_http_request(parser);
    if (m_req == NULL) {
        ret = ERROR_HTTP_GENERATE_REQUEST;
        log_error("generate http request failed. ret=%d", ret);
        return ret;
    }

    log_trace("ts recv. vhost=%s, app=%s, stream=%s", m_req->vhost.c_str(), m_req->app.c_str(), m_req->stream.c_str());

    get_config_value();

    if (!m_enable) {
        ret = ERROR_HTTP_TS_RECV_REJECT;
        return ret;
    }

    m_source = lms_source_manager::instance()->addSource(m_req);

    if (!m_source->add_reload_conn(m_conn)) {
        ret = ERROR_SOURCE_ADD_RELOAD;
        return ret;
    }

    if (!m_source->onPublish(m_conn->getEvent(), m_is_edge)) {
        ret = ERROR_SOURCE_ONPUBLISH;
        return ret;
    }

    m_source->start_external();

    m_demuxer = GetCodecTsDemuxer();
    m_demuxer->initialize(m_pkt_size);
    m_demuxer->setHandler(TS_DEMUXER_CALLBACK(&lms_http_ts_recv::onRecvMessage));

    return ret;
}

int lms_http_ts_recv::start()
{
    int ret = ERROR_SUCCESS;

    m_conn->setReadTimeOut(m_timeout);
    m_conn->setWriteTimeOut(-1);

    if ((ret = response_http_header()) != ERROR_SUCCESS) {
        if (ret != SOCKET_EAGAIN) {
            return ret;
        }
    }

    return ERROR_SUCCESS;
}

int lms_http_ts_recv::service()
{
    int ret = ERROR_SUCCESS;

    while (!m_reader->empty()) {
        MemoryChunk *chunk = DMemPool::instance()->getMemory(m_pkt_size);
        DSharedPtr<MemoryChunk> payload = DSharedPtr<MemoryChunk>(chunk);
        payload->length = m_pkt_size;

        if ((ret = m_reader->readBody(payload->data, m_pkt_size)) != ERROR_SUCCESS) {
            log_error_eagain(ret, ERROR_TS_READ_BODY, "http read ts body failed. ret=%d", ret);
            return ret;
        }

        if (m_demuxer->demuxer(payload->data) != ERROR_SUCCESS) {
            ret = ERROR_TS_DEMUXER;
            return ret;
        }
    }

    return ret;
}

bool lms_http_ts_recv::reload()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        return false;
    }

    m_enable = config->get_ts_recv_enable(m_req);
    if (!m_enable) {
        return false;
    }

    int timeout = config->get_http_timeout(m_req);
    m_timeout = timeout * 1000 * 1000;
    m_conn->setReadTimeOut(m_timeout);

    return true;
}

void lms_http_ts_recv::release()
{
    if (m_source) {
        m_source->stop_external();
        m_source->onUnpublish();
        m_source->del_reload_conn(m_conn);
    }
}

void lms_http_ts_recv::get_config_value()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        return;
    }

    m_enable = config->get_ts_recv_enable(m_req);
    m_is_edge = config->get_proxy_enable(m_req);

    int timeout = config->get_http_timeout(m_req);
    m_timeout = timeout * 1000 * 1000;
}

int lms_http_ts_recv::response_http_header()
{
    int ret = ERROR_SUCCESS;

    DHttpHeader header;
    header.setServer(LMS_VERSION);

    DString str = header.getResponseString(200);

    MemoryChunk *chunk = DMemPool::instance()->getMemory(str.size());
    DSharedPtr<MemoryChunk> response = DSharedPtr<MemoryChunk>(chunk);

    memcpy(response->data, str.data(), str.size());
    response->length = str.size();

    if ((ret = m_conn->write(response, str.size())) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_HTTP_SEND_RESPONSE_HEADER, "ts recv write http header failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int lms_http_ts_recv::onRecvMessage(TsDemuxerPacket pkt)
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

int lms_http_ts_recv::create_aac_message(TsDemuxerPacket &pkt)
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

int lms_http_ts_recv::create_mp3_message(TsDemuxerPacket &pkt)
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

int lms_http_ts_recv::create_avc_message(TsDemuxerPacket &pkt)
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

int lms_http_ts_recv::create_hevc_message(TsDemuxerPacket &pkt)
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

int lms_http_ts_recv::process_message(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if (m_is_edge) {
        return m_source->proxyMessage(msg);
    } else if (msg->is_audio()) {
        return m_source->onAudio(msg);
    } else if (msg->is_video()) {
        return m_source->onVideo(msg);
    } else {
        return m_source->onMetadata(msg);
    }

    return ret;
}
