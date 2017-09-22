#include "rtmp_server.hpp"
#include "DGlobal.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"

rtmp_server::rtmp_server(DTcpSocket *socket)
    : m_socket(socket)
    , m_in_chunk_size(128)
    , m_out_chunk_size(128)
    , m_player_buffer_length(1000)
    , m_objectEncoding(0)
    , m_stream_id(1)
    , m_av_handler(NULL)
    , m_metadata_handler(NULL)
    , m_connect_notify_handler(NULL)
    , m_play_verify_handler(NULL)
    , m_publish_verify_handler(NULL)
    , m_play_start_handler(NULL)
    , m_type(HandShake)
{
    m_hs = new rtmp_handshake(m_socket);
    m_ch = new RtmpChunk(m_socket);
    m_req = new kernel_request();
}

rtmp_server::~rtmp_server()
{
    DFree(m_hs);
    DFree(m_ch);
    DFree(m_req);
}

int rtmp_server::service()
{
    int ret = ERROR_SUCCESS;

    while (true) {
        switch (m_type) {
        case HandShake:
            ret = handshake();
            break;
        case RecvChunk:
            ret = read_chunk();
            break;
        default:
            break;
        }

        if (ret != ERROR_SUCCESS) {
            break;
        }
    }

    return ret;
}

void rtmp_server::set_chunk_size(dint32 chunk_size)
{
    m_out_chunk_size = chunk_size;
}

void rtmp_server::set_in_ack_size(int ack_size)
{
    in_ack_size.window = ack_size;
}

kernel_request *rtmp_server::get_request()
{
    return m_req;
}

int rtmp_server::send_av_data(RtmpMessage *msg)
{
    int ret = ERROR_SUCCESS;

    msg->header.stream_id = m_stream_id;

    if ((ret = m_ch->send_message(msg)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ret, "send rtmp video/audio message failed. type=%d, ret=%d", msg->header.message_type, ret);
        return ret;
    }

    return ret;
}

void rtmp_server::set_av_handler(RtmpAVHandler handler)
{
    m_av_handler = handler;
}

void rtmp_server::set_metadata_handler(RtmpAVHandler handler)
{
    m_metadata_handler = handler;
}

void rtmp_server::set_connect_verify_handler(RtmpNotifyHandler handler)
{
    m_connect_notify_handler = handler;
}

void rtmp_server::set_play_verify_handler(RtmpNotifyHandler handler)
{
    m_play_verify_handler = handler;
}

void rtmp_server::set_publish_verify_handler(RtmpNotifyHandler handler)
{
    m_publish_verify_handler = handler;
}

void rtmp_server::set_play_start(RtmpVerifyHandler handler)
{
    m_play_start_handler = handler;
}

int rtmp_server::send_publish_notify()
{
    int ret = ERROR_SUCCESS;

    OnStatusCallPacket *pkt = new OnStatusCallPacket();
    DAutoFree(OnStatusCallPacket, pkt);

    pkt->header.stream_id = m_stream_id;

    std::string description = m_req->stream + " is now published.";

    pkt->data.setValue(StatusLevel, new AMF0String(StatusLevelStatus));
    pkt->data.setValue(StatusCode, new AMF0String(StatusCodeStreamPublishNotify));
    pkt->data.setValue(StatusDescription, new AMF0String(description));
    pkt->data.setValue(StatusDetails, new AMF0String(m_req->stream));
    pkt->data.setValue(StatusClientId, new AMF0String(RTMP_SIG_CLIENT_ID));

    if ((ret = send_message(pkt)) != ERROR_SUCCESS) {
        log_error("send PublishNotify packet failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

dint64 rtmp_server::get_player_buffer_length()
{
    return m_player_buffer_length;
}

int rtmp_server::response_connect(bool allow)
{
    int ret = ERROR_SUCCESS;

    log_info("start response connect");

    if (!allow) {
        if ((ret = send_connect_refuse()) != ERROR_SUCCESS) {
            return ret;
        }

        ret = ERROR_RTMP_CONNECT_REFUSED;
        log_warn("refused rtmp connect request. tcUrl=%s", m_req->tcUrl.c_str());

        return ret;
    }

    if ((ret = send_window_ack_size(RTMP_DEFAULT_ACKWINKOW_SIZE)) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = send_connect_response()) != ERROR_SUCCESS) {
        return ret;
    }

    log_info("response connect success");

    return ret;
}

int rtmp_server::response_publish(bool allow)
{
    int ret = ERROR_SUCCESS;

    if (!allow) {
        ret = ERROR_RTMP_PUBLISH_REFUSED;
        log_warn("refuse publish. ret=%d", ret);
        return ret;
    }

    if ((ret = send_chunk_size(m_out_chunk_size)) != ERROR_SUCCESS) {
        return ret;
    }

    m_ch->set_out_chunk_size(m_out_chunk_size);

    // onFCPublish(NetStream.Publish.Start)
    OnStatusCallPacket* pkt1 = new OnStatusCallPacket();
    DAutoFree(OnStatusCallPacket, pkt1);

    pkt1->command_name = RTMP_AMF0_COMMAND_ON_FC_PUBLISH;
    pkt1->header.stream_id = m_stream_id;
    pkt1->data.setValue(StatusCode, new AMF0String(StatusCodePublishStart));
    pkt1->data.setValue(StatusDescription, new AMF0String("Started publishing stream."));

    if ((ret = send_message(pkt1)) != ERROR_SUCCESS) {
        log_error("send rtmp onFCPublish(NetStream.Publish.Start) packet failed. ret=%d", ret);
        return ret;
    }

    // onStatus(NetStream.Publish.Start)
    OnStatusCallPacket* pkt2 = new OnStatusCallPacket();
    DAutoFree(OnStatusCallPacket, pkt2);

    pkt2->header.stream_id = m_stream_id;
    pkt2->data.setValue(StatusLevel, new AMF0String(StatusLevelStatus));
    pkt2->data.setValue(StatusCode, new AMF0String(StatusCodePublishStart));
    pkt2->data.setValue(StatusDescription, new AMF0String("Started publishing stream."));
    pkt2->data.setValue(StatusClientId, new AMF0String(RTMP_SIG_CLIENT_ID));

    if ((ret = send_message(pkt2)) != ERROR_SUCCESS) {
        log_error("send rtmp onStatus(NetStream.Publish.Start) packet failed. ret=%d", ret);
        return ret;
    }

    log_trace("response publish success");

    return ret;
}

int rtmp_server::response_play(bool allow)
{
    int ret = ERROR_SUCCESS;

    if (!allow) {
        ret = ERROR_RTMP_PLAY_REFUSED;
        return ret;
    }

    if ((ret = send_chunk_size(m_out_chunk_size)) != ERROR_SUCCESS) {
        return ret;
    }
    m_ch->set_out_chunk_size(m_out_chunk_size);

    // StreamBegin
    UserControlPacket* pkt1 = new UserControlPacket();
    DAutoFree(UserControlPacket, pkt1);

    pkt1->event_type = SrcPCUCStreamBegin;
    pkt1->event_data = 1;

    if ((ret = send_message(pkt1)) != ERROR_SUCCESS) {
        log_error("send rtmp PCUC(StreamBegin) packet failed. ret=%d", ret);
        return ret;
    }

    // onStatus(NetStream.Play.Reset)
    OnStatusCallPacket* pkt2 = new OnStatusCallPacket();
    DAutoFree(OnStatusCallPacket, pkt2);

    pkt2->header.stream_id = m_stream_id;

    pkt2->data.setValue(StatusLevel, new AMF0String(StatusLevelStatus));
    pkt2->data.setValue(StatusCode, new AMF0String(StatusCodeStreamReset));
    pkt2->data.setValue(StatusDescription, new AMF0String("Playing and resetting stream."));
    pkt2->data.setValue(StatusDetails, new AMF0String("stream"));
    pkt2->data.setValue(StatusClientId, new AMF0String(RTMP_SIG_CLIENT_ID));

    if ((ret = send_message(pkt2)) != ERROR_SUCCESS) {
        log_error("send rtmp onStatus(NetStream.Play.Reset) packet failed. ret=%d", ret);
        return ret;
    }

    // onStatus(NetStream.Play.Start)
    OnStatusCallPacket* pkt3 = new OnStatusCallPacket();
    DAutoFree(OnStatusCallPacket, pkt3);

    pkt3->header.stream_id = m_stream_id;

    pkt3->data.setValue(StatusLevel, new AMF0String(StatusLevelStatus));
    pkt3->data.setValue(StatusCode, new AMF0String(StatusCodeStreamStart));
    pkt3->data.setValue(StatusDescription, new AMF0String("Started playing stream."));
    pkt3->data.setValue(StatusDetails, new AMF0String("stream"));
    pkt3->data.setValue(StatusClientId, new AMF0String(RTMP_SIG_CLIENT_ID));

    if ((ret = send_message(pkt3)) != ERROR_SUCCESS) {
        log_error("send rtmp onStatus(NetStream.Play.Start) packet failed. ret=%d", ret);
        return ret;
    }

    // |RtmpSampleAccess(false, false)
    SampleAccessPacket *pkt4 = new SampleAccessPacket();
    DAutoFree(SampleAccessPacket, pkt4);

    pkt4->header.stream_id = m_stream_id;
    pkt4->audio_sample_access = true;
    pkt4->video_sample_access = true;

    if ((ret = send_message(pkt4)) != ERROR_SUCCESS) {
        log_error("send rtmp |RtmpSampleAccess(false, false) packet failed. ret=%d", ret);
        return ret;
    }

    // onStatus(NetStream.Data.Start)
    OnStatusDataPacket* pkt5 = new OnStatusDataPacket();
    DAutoFree(OnStatusDataPacket, pkt5);

    pkt5->header.stream_id = m_stream_id;
    pkt5->data.setValue(StatusCode, new AMF0String(StatusCodeDataStart));

    if ((ret = send_message(pkt5)) != ERROR_SUCCESS) {
        log_error("send rtmp onStatus(NetStream.Data.Start) packet failed. ret=%d", ret);
        return ret;
    }

    if ((ret = send_publish_notify()) != ERROR_SUCCESS) {
        log_error("send rtmp NetStream.play.PublishNotify packet failed. ret=%d", ret);
        return ret;
    }

    log_trace("response play success");

    if (m_play_start_handler) {
        if (!m_play_start_handler(m_req)) {
            ret = ERROR_RTMP_PLAY_START_FAILED;
            log_error("rtmp play start failed. ret=%d", ret);
            return ret;
        }
    }

    return ret;
}

int rtmp_server::handshake()
{
    int ret = ERROR_SUCCESS;

    if ((ret = m_hs->handshake_with_client()) != ERROR_SUCCESS) {
        log_error_eagain(ret, ret, "handshake with client failed. ret=%d", ret);
        return ret;
    }

    if (m_hs->completed()) {
        log_info("handshake with client success");
        m_type = RecvChunk;
        DFree(m_hs);
    }

    return ret;
}

int rtmp_server::read_chunk()
{
    int ret = ERROR_SUCCESS;

    if ((ret = m_ch->read_chunk()) != ERROR_SUCCESS) {
        return ret;
    }

    if (m_ch->entired()) {
        if ((ret = decode_message()) != ERROR_SUCCESS) {
            return ret;
        }
    }

    return ret;
}

int rtmp_server::decode_message()
{
    int ret = ERROR_SUCCESS;

    RtmpMessage *msg = m_ch->get_message();
    DAutoFree(RtmpMessage, msg);

    if((ret = send_acknowledgement()) != ERROR_SUCCESS) {
        return ret;
    }

    if (msg->is_amf0_command() || msg->is_amf3_command()) {
        char *data = msg->payload->data;
        int len = msg->payload->length;

        if (msg->is_amf3_command() && msg->payload->length >= 1) {
            data = msg->payload->data + 1;
            len = msg->payload->length - 1;
        }

        DString command;
        if ((ret = get_command_name(data, len, command)) != ERROR_SUCCESS) {
            log_error("get rtmp amf0/amf3 command name failed. ret=%d", ret);
            return ret;
        }

        if (command == RTMP_AMF0_COMMAND_CONNECT) {
            return process_connect_app(msg);
        } else if (command == RTMP_AMF0_COMMAND_CREATE_STREAM) {
            return process_create_stream(msg);
        } else if (command == RTMP_AMF0_COMMAND_PUBLISH) {
            return process_publish(msg);
        } else if (command == RTMP_AMF0_COMMAND_RELEASE_STREAM) {
            return process_release_stream(msg);
        } else if (command == RTMP_AMF0_COMMAND_FC_PUBLISH) {
            return process_FCPublish(msg);
        } else if (command == RTMP_AMF0_COMMAND_PLAY) {
            return process_play(msg);
        } else if (command == RTMP_AMF0_COMMAND_CLOSE_STREAM) {
            return process_close_stream(msg);
        } else if (command == RTMP_AMF0_COMMAND_UNPUBLISH) {
            return process_FCUnpublish(msg);
        }
    } else if (msg->is_amf0_data() || msg->is_amf3_data()) {
        char *data = msg->payload->data;
        int len = msg->payload->length;

        if (msg->is_amf3_data() && msg->payload->length >= 1) {
            data = msg->payload->data + 1;
            len = msg->payload->length - 1;
        }

        DString command;
        if ((ret = get_command_name(data, len, command)) != ERROR_SUCCESS) {
            log_error("get rtmp amf data name failed. ret=%d", ret);
            return ret;
        }

        if(command == RTMP_AMF0_DATA_SET_DATAFRAME || command == RTMP_AMF0_DATA_ON_METADATA) {
            return process_metadata(msg);
        }
    } else if (msg->is_set_chunk_size()) {
        return process_set_chunk_size(msg);
    } else if (msg->is_user_control_message()) {
        return process_user_control(msg);
    } else if (msg->is_window_acknowledgement_size()) {
        return process_window_ackledgement_size(msg);
    } else if (msg->is_video() || msg->is_audio()) {
        return process_video_audio(msg);
    } else if (msg->is_aggregate()) {
        return process_aggregate(msg);
    }

    return ret;
}

int rtmp_server::process_connect_app(RtmpMessage *msg)
{
    int ret = ERROR_SUCCESS;

    log_info("recv connect app");

    ConnectAppPacket *pkt = new ConnectAppPacket();
    DAutoFree(ConnectAppPacket, pkt);
    pkt->copy(msg);

    if ((ret = pkt->decode()) != ERROR_SUCCESS) {
        log_error("decode rtmp connect packet failed. ret=%d", ret);
        return ret;
    }

    m_req->set_tcUrl(pkt->tcUrl);
    m_req->pageUrl = pkt->pageUrl;
    m_req->swfUrl = pkt->swfUrl;
    m_objectEncoding = pkt->objectEncoding;

    if (m_connect_notify_handler) {
        m_connect_notify_handler(m_req);
    }

    return ret;
}

int rtmp_server::process_create_stream(RtmpMessage *msg)
{
    int ret = ERROR_SUCCESS;

    log_trace("start create createStream");

    CreateStreamPacket *pkt  = new CreateStreamPacket();
    DAutoFree(CreateStreamPacket, pkt);
    pkt->copy(msg);

    if ((ret = pkt->decode()) != ERROR_SUCCESS) {
        log_error("decode rtmp createStream packet failed. ret=%d", ret);
        return ret;
    }

    CreateStreamResPacket *res_pkt = new CreateStreamResPacket(pkt->transaction_id, m_stream_id);
    DAutoFree(CreateStreamResPacket, res_pkt);

    if ((ret = send_message(res_pkt)) != ERROR_SUCCESS) {
        log_error("send rtmp createStream response packet failed. ret=%d", ret);
        return ret;
    }

    log_trace("response createStream success");

    return ret;
}

int rtmp_server::process_publish(RtmpMessage *msg)
{
    int ret = ERROR_SUCCESS;

    // publish的payload需要解出流名
    PublishPacket *pkt  = new PublishPacket();
    DAutoFree(PublishPacket, pkt);
    pkt->copy(msg);

    if ((ret = pkt->decode()) != ERROR_SUCCESS) {
        log_error("decode rtmp publish packet failed. ret=%d", ret);
        return ret;
    }

    if (m_stream_id != pkt->header.stream_id) {
        log_warn("rtmp publish stream_id must be %d, actual=%d", m_stream_id, pkt->header.stream_id);
    }

    m_req->set_stream(pkt->stream_name);

    if (m_publish_verify_handler) {
        m_publish_verify_handler(m_req);
    }

    return ret;
}

int rtmp_server::process_play(RtmpMessage *msg)
{
    int ret = ERROR_SUCCESS;

    log_trace("start play");

    // play的payload需要解出流名
    PlayPacket *pkt  = new PlayPacket();
    DAutoFree(PlayPacket, pkt);
    pkt->copy(msg);

    if ((ret = pkt->decode()) != ERROR_SUCCESS) {
        log_error("decode rtmp play packet failed. ret=%d", ret);
        return ret;
    }

    if (m_stream_id != pkt->header.stream_id) {
        log_warn("rtmp play stream_id must be %d, actual=%d", m_stream_id, pkt->header.stream_id);
    }

    m_req->set_stream(pkt->stream_name);

    if (m_play_verify_handler) {
        m_play_verify_handler(m_req);
    }

    return ret;
}

int rtmp_server::process_release_stream(RtmpMessage *msg)
{
    int ret = ERROR_SUCCESS;

    log_trace("start create releaseStream");

    FmleStartPacket *pkt = new FmleStartPacket();
    DAutoFree(FmleStartPacket, pkt);
    pkt->copy(msg);

    if ((ret = pkt->decode()) != ERROR_SUCCESS) {
        log_error("decode rtmp releaseStream packet failed. ret=%d", ret);
        return ret;
    }

    FmleStartResPacket *pkt1 = new FmleStartResPacket(pkt->transaction_id);
    DAutoFree(FmleStartResPacket, pkt1);

    if ((ret = send_message(pkt1)) != ERROR_SUCCESS) {
        log_error("send rtmp releaseStream response packet failed. ret=%d", ret);
        return ret;
    }

    log_trace("response releaseStream success");

    return ret;
}

int rtmp_server::process_FCPublish(RtmpMessage *msg)
{
    int ret = ERROR_SUCCESS;

    log_trace("start FCPublish");

    FmleStartPacket *pkt = new FmleStartPacket();
    DAutoFree(FmleStartPacket, pkt);
    pkt->copy(msg);

    if ((ret = pkt->decode()) != ERROR_SUCCESS) {
        log_error("decode rtmp FCPublish packet failed. ret=%d", ret);
        return ret;
    }

    FmleStartResPacket *pkt1 = new FmleStartResPacket(pkt->transaction_id);
    DAutoFree(FmleStartResPacket, pkt1);

    if ((ret = send_message(pkt1)) != ERROR_SUCCESS) {
        log_error("send FCPublish response packet failed. ret=%d", ret);
        return ret;
    }

    log_trace("response FCPublish success");

    return ret;
}

int rtmp_server::process_FCUnpublish(RtmpMessage *msg)
{
    int ret = ERROR_SUCCESS;

    FmleStartPacket *pkt = new FmleStartPacket();
    DAutoFree(FmleStartPacket, pkt);
    pkt->copy(msg);

    if ((ret = pkt->decode()) != ERROR_SUCCESS) {
        log_error("decode rtmp FCUnpublish failed. ret=%d", ret);
        return ret;
    }

    // response onFCUnpublish(NetStream.unpublish.Success)
    OnStatusCallPacket* pkt1 = new OnStatusCallPacket();
    DAutoFree(OnStatusCallPacket, pkt1);

    pkt1->command_name = RTMP_AMF0_COMMAND_ON_FC_UNPUBLISH;
    pkt1->data.setValue(StatusCode, new AMF0String(StatusCodeUnpublishSuccess));
    pkt1->data.setValue(StatusDescription, new AMF0String("Stop publishing stream."));

    if ((ret = send_message(pkt1)) != ERROR_SUCCESS) {
        log_error("send rtmp onFCUnpublish(NetStream.unpublish.Success) packet failed. ret=%d", ret);
        return ret;
    }

    // FCUnpublish response
    FmleStartResPacket *pkt2 = new FmleStartResPacket(pkt->transaction_id);
    DAutoFree(FmleStartResPacket, pkt2);

    if ((ret = send_message(pkt2)) != ERROR_SUCCESS) {
        log_error("send rmtp FCUnpublish response packet failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int rtmp_server::process_close_stream(RtmpMessage *msg)
{
    int ret = ERROR_SUCCESS;

    CloseStreamPacket *pkt = new CloseStreamPacket();
    DAutoFree(CloseStreamPacket, pkt);
    pkt->copy(msg);

    if ((ret = pkt->decode()) != ERROR_SUCCESS) {
        log_error("decode rtmp closeStream packet failed. ret=%d", ret);
        return ret;
    }

    // response onStatus(NetStream.Unpublish.Success)
    OnStatusCallPacket* pkt1 = new OnStatusCallPacket();
    DAutoFree(OnStatusCallPacket, pkt1);

    pkt1->data.setValue(StatusLevel, new AMF0String(StatusLevelStatus));
    pkt1->data.setValue(StatusCode, new AMF0String(StatusCodeUnpublishSuccess));
    pkt1->data.setValue(StatusDescription, new AMF0String("Stream is now unpublished"));
    pkt1->data.setValue(StatusClientId, new AMF0String(RTMP_SIG_CLIENT_ID));

    if ((ret = send_message(pkt1)) != ERROR_SUCCESS) {
        log_error("send rtmp onStatus(NetStream.Unpublish.Success) packet failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int rtmp_server::process_set_chunk_size(RtmpMessage *msg)
{
    int ret = ERROR_SUCCESS;

    SetChunkSizePacket *pkt = new SetChunkSizePacket();
    DAutoFree(SetChunkSizePacket, pkt);
    pkt->copy(msg);

    if ((ret = pkt->decode()) != ERROR_SUCCESS) {
        log_error("decode rtmp Set_Chunk_Size packet failed. ret=%d", ret);
        return ret;
    }

    m_in_chunk_size = pkt->chunk_size;
    m_ch->set_in_chunk_size(m_in_chunk_size);

    log_trace("peer_chunk_size=%d", m_in_chunk_size);
    return ret;
}

int rtmp_server::process_window_ackledgement_size(RtmpMessage *msg)
{
    int ret = ERROR_SUCCESS;

    SetWindowAckSizePacket *pkt = new SetWindowAckSizePacket();
    DAutoFree(SetWindowAckSizePacket, pkt);
    pkt->copy(msg);

    if ((ret = pkt->decode()) != ERROR_SUCCESS) {
        log_error("decode rmtp Window_Ackledgement_Size packet failed. ret=%d", ret);
        return ret;
    }

    if (pkt->ackowledgement_window_size > 0) {
        in_ack_size.window = pkt->ackowledgement_window_size;
    }

    log_trace("peer_window_acknowledgement_size=%d", pkt->ackowledgement_window_size);

    return ret;
}

int rtmp_server::process_user_control(RtmpMessage *msg)
{
    int ret = ERROR_SUCCESS;

    UserControlPacket *pkt = new UserControlPacket();
    DAutoFree(UserControlPacket, pkt);
    pkt->copy(msg);

    if ((ret = pkt->decode()) != ERROR_SUCCESS) {
        log_error("decode rtmp UserControl packet failed. ret=%d", ret);
        return ret;
    }

    if (pkt->event_type == SrcPCUCSetBufferLength) {
        m_player_buffer_length = pkt->extra_data;
    }

    return ret;
}

int rtmp_server::process_metadata(RtmpMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if (!m_metadata_handler) {
        return ret;
    }

    OnMetaDataPacket *pkt = new OnMetaDataPacket();
    DAutoFree(OnMetaDataPacket, pkt);
    pkt->copy(msg);

    if ((ret = pkt->decode()) != ERROR_SUCCESS) {
        log_error("decode rtmp metadata message failed. ret=%d", ret);
        return ret;
    }

    return m_metadata_handler(pkt);
}

int rtmp_server::process_video_audio(RtmpMessage *msg)
{
    if (!m_av_handler) {
        return ERROR_SUCCESS;
    }

    return m_av_handler(msg);
}

int rtmp_server::process_aggregate(RtmpMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if (!m_av_handler) {
        return ret;
    }

    if (msg->payload->length <= 0) {
        return ret;
    }

    DStream stream(msg->payload->data, msg->payload->length);

    bool first = true;
    dint64 base_time = 0;

    while (stream.left() > 0) {
        dint8 type;
        if (!stream.read1Bytes(type)) {
            ret = ERROR_RTMP_AGGREGATE;
            log_error("invalid rtmp aggregate message type. ret=%d", ret);
        }

        dint32 data_size;
        if (!stream.read3Bytes(data_size) || (data_size < 0)) {
            ret = ERROR_RTMP_AGGREGATE;
            log_error("invalid rtmp aggregate message size. ret=%d", ret);
            return ret;
        }

        dint32 timestamp;
        if (!stream.read3Bytes(timestamp)) {
            ret = ERROR_RTMP_AGGREGATE;
            log_error("invalid rtmp aggregate message time. ret=%d", ret);
            return ret;
        }

        dint8 time_h;
        if (!stream.read1Bytes(time_h)) {
            ret = ERROR_RTMP_AGGREGATE;
            log_error("invalid rtmp aggregate message time(high). ret=%d", ret);
            return ret;
        }

        timestamp |= (dint32)time_h << 24;
        timestamp &= 0x7FFFFFFF;

        if (first) {
            base_time = timestamp;
            first = false;
        }

        dint32 stream_id;
        if (!stream.read3Bytes(stream_id)) {
            ret = ERROR_RTMP_AGGREGATE;
            log_error("invalid rtmp aggregate message stream_id. ret=%d", ret);
            return ret;
        }

        if ((data_size > 0) && (stream.left() < data_size)) {
            ret = ERROR_RTMP_AGGREGATE;
            log_error("invalid rtmp aggregate message data. ret=%d", ret);
            return ret;
        }

        RtmpMessage _msg;
        _msg.header.message_type = type;
        _msg.header.payload_length = data_size;
        _msg.header.timestamp_delta = timestamp;
        _msg.header.timestamp = msg->header.timestamp + (timestamp - base_time);
        _msg.header.stream_id = stream_id;
        _msg.header.perfer_cid = msg->header.perfer_cid;

        if (data_size > 0) {
            MemoryChunk *chunk = DMemPool::instance()->getMemory(data_size);
            _msg.payload = DSharedPtr<MemoryChunk>(chunk);
            _msg.payload->length = data_size;

            stream.readBytes(_msg.payload->data, data_size);
        }

        if (stream.left() < 4) {
            ret = ERROR_RTMP_AGGREGATE;
            log_error("invalid rtmp aggregate message previous tag size. ret=%d", ret);
            return ret;
        }

        if (_msg.is_video() || _msg.is_audio()) {
            if ((ret = m_av_handler(&_msg)) != ERROR_SUCCESS) {
                return ret;
            }
        }
    }

    return ret;
}

int rtmp_server::send_connect_response()
{
    int ret = ERROR_SUCCESS;

    ConnectAppResPacket *pkt = new ConnectAppResPacket();
    DAutoFree(ConnectAppResPacket, pkt);

    pkt->props.setValue("fmsVer", new AMF0String("FMS/"RTMP_SIG_FMS_VER));
    pkt->props.setValue("capabilities", new AMF0Number(127));
    pkt->props.setValue("mode", new AMF0Number(1));

    pkt->info.setValue(StatusLevel, new AMF0String(StatusLevelStatus));
    pkt->info.setValue(StatusCode, new AMF0String(StatusCodeConnectSuccess));
    pkt->info.setValue(StatusDescription, new AMF0String("Connection succeeded"));
    pkt->info.setValue("objectEncoding", new AMF0Number(m_objectEncoding));

    if ((ret = send_message(pkt)) != ERROR_SUCCESS) {
        log_error("send connect response packet failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int rtmp_server::send_connect_refuse()
{
    int ret = ERROR_SUCCESS;

    OnErrorPacket *pkt = new OnErrorPacket();
    DAutoFree(OnErrorPacket, pkt);

    pkt->data.setValue(StatusLevel, new AMF0String(StatusLevelError));
    pkt->data.setValue(StatusCode, new AMF0String(StatusCodeConnectRejected));
    pkt->data.setValue(StatusDescription, new AMF0String("connect refused"));

    if ((ret = send_message(pkt)) != ERROR_SUCCESS) {
        log_error("send connect refuse packet failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int rtmp_server::send_window_ack_size(dint32 ack_size)
{
    int ret = ERROR_SUCCESS;

    SetWindowAckSizePacket *pkt = new SetWindowAckSizePacket();
    DAutoFree(SetWindowAckSizePacket, pkt);
    pkt->ackowledgement_window_size = ack_size;

    if ((ret = send_message(pkt)) != ERROR_SUCCESS) {
        log_error("send window ack size failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int rtmp_server::send_chunk_size(dint32 chunk_size)
{
    int ret = ERROR_SUCCESS;

    SetChunkSizePacket *pkt = new SetChunkSizePacket();
    DAutoFree(SetChunkSizePacket, pkt);
    pkt->chunk_size = chunk_size;

    if ((ret = send_message(pkt)) != ERROR_SUCCESS) {
        log_error("send chunk size failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int rtmp_server::send_acknowledgement()
{
    int ret = ERROR_SUCCESS;

    if (in_ack_size.window <= 0) {
        return ret;
    }

    // ignore when delta bytes not exceed half of window(ack size).
    duint64 delta = m_socket->getTotalReadSize() - in_ack_size.nb_recv_bytes;
    if (delta < (duint64)in_ack_size.window / 2) {
        return ret;
    }
    in_ack_size.nb_recv_bytes = m_socket->getTotalReadSize();

    // when the sequence number overflow, reset it.
    duint64 sequence_number = in_ack_size.sequence_number + delta;
    if (sequence_number > 0xf0000000) {
        sequence_number = delta;
    }
    in_ack_size.sequence_number = sequence_number;

    AcknowledgementPacket *pkt = new AcknowledgementPacket();
    DAutoFree(AcknowledgementPacket, pkt);
    pkt->sequence_number = sequence_number;

    if ((ret = send_message(pkt)) != ERROR_SUCCESS) {
        log_error("send acknowledgement failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int rtmp_server::get_command_name(char *data, int len, DString &name)
{
    int ret = ERROR_SUCCESS;

    RtmpPacket *pkt = new RtmpPacket();
    DAutoFree(RtmpPacket, pkt);
    if (pkt->decode_packet(data, len) != ERROR_SUCCESS) {
        return ret;
    }
    if (!pkt->findString(1, name)) {
        return ERROR_RTMP_COMMAND_NAME_NOTFOUND;
    }

    return ret;
}

int rtmp_server::send_message(RtmpMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if ((ret = m_ch->send_message(msg)) != ERROR_SUCCESS) {
        if (ret != SOCKET_EAGAIN) {
            return ret;
        }
    }

    return ERROR_SUCCESS;
}

