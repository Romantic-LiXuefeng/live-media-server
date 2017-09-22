#include "rtmp_client.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"

rtmp_client::rtmp_client(DTcpSocket *socket)
    : m_socket(socket)
    , m_in_chunk_size(128)
    , m_out_chunk_size(128)
    , m_player_buffer_length(1000)
    , m_stream_id(1)
    , m_publish(false)
    , m_av_handler(NULL)
    , m_metadata_handler(NULL)
    , m_publish_start_handler(NULL)
    , m_play_start_handler(NULL)
    , m_type(HandShake)
{
    m_hs = new rtmp_handshake(m_socket);
    m_hs->set_handshake_with_server_type(false);

    m_ch = new RtmpChunk(m_socket);
    m_req = new kernel_request();
}

rtmp_client::~rtmp_client()
{
    DFree(m_hs);
    DFree(m_ch);
    DFree(m_req);
}

int rtmp_client::service()
{
    int ret = ERROR_SUCCESS;

    while (true) {
        switch (m_type) {
        case HandShake:
            ret = handshake();
            break;
        case Connect:
            ret = connect_app();
            break;
        case CreateStream:
            ret = create_stream();
            break;
        case Publish:
            ret = publish();
            break;
        case Play:
            ret = play();
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

void rtmp_client::set_chunk_size(dint32 chunk_size)
{
    m_out_chunk_size = chunk_size;
}

void rtmp_client::set_in_ack_size(dint32 ack_size)
{
    in_ack_size.window = ack_size;
}

void rtmp_client::set_buffer_length(dint64 len)
{
    m_player_buffer_length = len;
}

int rtmp_client::start_rtmp(bool publish, kernel_request *req)
{
    m_publish = publish;
    m_req->copy(req);

    return service();
}

int rtmp_client::stop_rtmp()
{
    int ret = ERROR_SUCCESS;

    CloseStreamPacket *pkt = new CloseStreamPacket();
    DAutoFree(CloseStreamPacket, pkt);

    if ((ret = send_message(pkt)) != ERROR_SUCCESS) {
        log_error("send rtmp closeStream packet failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int rtmp_client::send_av_data(RtmpMessage *msg)
{
    int ret = ERROR_SUCCESS;

    msg->header.stream_id = m_stream_id;

    if ((ret = m_ch->send_message(msg)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ret, "send audio/video message to server failed. type=%d, ret=%d", msg->header.message_type, ret);
        return ret;
    }

    return ret;
}

void rtmp_client::set_av_handler(RtmpAVHandler handler)
{
    m_av_handler = handler;
}

void rtmp_client::set_metadata_handler(RtmpAVHandler handler)
{
    m_metadata_handler = handler;
}

void rtmp_client::set_publish_start_handler(RtmpVerifyHandler handler)
{
    m_publish_start_handler = handler;
}

void rtmp_client::set_play_start_handler(RtmpVerifyHandler handler)
{
    m_play_start_handler = handler;
}

int rtmp_client::read_chunk()
{
    int ret = ERROR_SUCCESS;

    if ((ret = m_ch->read_chunk()) != ERROR_SUCCESS) {
        return ret;
    }

    if (m_ch->entired()) {
        return decode_message();
    }

    return ret;
}

int rtmp_client::decode_message()
{
    int ret = ERROR_SUCCESS;

    RtmpMessage *msg = m_ch->get_message();
    DAutoFree(RtmpMessage, msg);

    if ((ret = send_acknowledgement()) != ERROR_SUCCESS) {
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
        double transactionId;
        if ((ret = get_command_name_id(data, len, command, transactionId)) != ERROR_SUCCESS) {
            log_error("get rtmp amf0/amf3 command command_name and transactionId failed. ret=%d", ret);
            return ret;
        }

        // result/error packet
        if (command == RTMP_AMF0_COMMAND_RESULT || command == RTMP_AMF0_COMMAND_ERROR) {
            // find the call name
            if (m_requests.find(transactionId) != m_requests.end()) {
                DString request_name = m_requests[transactionId];

                if (request_name == RTMP_AMF0_COMMAND_CONNECT) {
                    return process_connect_response(msg);
                } else if (request_name == RTMP_AMF0_COMMAND_CREATE_STREAM) {
                    return process_create_stream_response(msg);
                }
            }
        } else if (command == RTMP_AMF0_COMMAND_ON_STATUS) {
            return process_command_onstatus(msg);
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
            log_error("get rmtp amf0/amf3 command name failed. ret=%d", ret);
            return ret;
        }

        if(command == RTMP_AMF0_DATA_SET_DATAFRAME || command == RTMP_AMF0_DATA_ON_METADATA) {
            return process_metadata(msg);
        }
    } else if (msg->is_set_chunk_size()) {
        return process_set_chunk_size(msg);
    } else if (msg->is_window_acknowledgement_size()) {
        return process_window_ackledgement_size(msg);
    } else if (msg->is_video() || msg->is_audio()) {
        return process_video_audio(msg);
    } else if (msg->is_aggregate()) {
        return process_aggregate(msg);
    }

    return ret;
}

int rtmp_client::handshake()
{
    int ret = ERROR_SUCCESS;

    if ((ret = m_hs->handshake_with_server()) != ERROR_SUCCESS) {
        log_error_eagain(ret, ret, "handshake with server failed. ret=%d", ret);
        return ret;
    }

    if (m_hs->completed()) {
        log_info("handshake with server success");
        m_type = Connect;
        DFree(m_hs);
    }

    return ret;
}

int rtmp_client::connect_app()
{
    int ret = ERROR_SUCCESS;

    ConnectAppPacket *pkt = new ConnectAppPacket();
    DAutoFree(ConnectAppPacket, pkt);

    pkt->command_object.setValue("app", new AMF0String(m_req->app));
    pkt->command_object.setValue("flashVer", new AMF0String("WIN 12,0,0,41"));
    pkt->command_object.setValue("swfUrl", new AMF0String(m_req->swfUrl));
    pkt->command_object.setValue("tcUrl", new AMF0String(m_req->tcUrl));
    pkt->command_object.setValue("fpad", new AMF0Boolean(false));
    pkt->command_object.setValue("capabilities", new AMF0Number(239));
    pkt->command_object.setValue("audioCodecs", new AMF0Number(3575));
    pkt->command_object.setValue("videoCodecs", new AMF0Number(252));
    pkt->command_object.setValue("videoFunction", new AMF0Number(1));
    pkt->command_object.setValue("objectEncoding", new AMF0Number(0));

    m_type = RecvChunk;
    m_requests[pkt->transaction_id] = pkt->command_name;

    if ((ret = send_message(pkt)) != ERROR_SUCCESS) {
        log_error("send connect packet failed. ret=%d", ret);
        return ret;
    }

    log_info("connect app success");

    return ret;
}

int rtmp_client::create_stream()
{
    int ret = ERROR_SUCCESS;

    CreateStreamPacket *pkt = new CreateStreamPacket();
    DAutoFree(CreateStreamPacket, pkt);

    m_type = RecvChunk;
    m_requests[pkt->transaction_id] = pkt->command_name;

    if ((ret = send_message(pkt)) != ERROR_SUCCESS) {
        log_error("send create stream packet failed. ret=%d", ret);
        return ret;
    }

    log_info("create stream success");

    return ret;
}

int rtmp_client::publish()
{
    int ret = ERROR_SUCCESS;

    PublishPacket *pkt = new PublishPacket();
    DAutoFree(PublishPacket, pkt);

    pkt->header.stream_id = m_stream_id;
    pkt->stream_name = m_req->stream;

    m_type = RecvChunk;

    if ((ret = send_message(pkt)) != ERROR_SUCCESS) {
        log_error("send publish packet failed. ret=%d", ret);
        return ret;
    }

    log_info("publish success");

    return ret;
}

int rtmp_client::play()
{
    int ret = ERROR_SUCCESS;

    if ((ret = send_buffer_length(m_player_buffer_length)) != ERROR_SUCCESS) {
        return ret;
    }

    PlayPacket *pkt = new PlayPacket();
    DAutoFree(PlayPacket, pkt);

    pkt->header.stream_id = m_stream_id;
    pkt->stream_name = m_req->stream;

    m_type = RecvChunk;

    if ((ret = send_message(pkt)) != ERROR_SUCCESS) {
        log_error("send play packet failed. ret=%d", ret);
        return ret;
    }

    log_info("play success");

    return ret;
}

int rtmp_client::process_connect_response(RtmpMessage *msg)
{
    int ret = ERROR_SUCCESS;

    log_info("start connect response");

    ConnectAppResPacket *pkt = new ConnectAppResPacket();
    DAutoFree(ConnectAppResPacket, pkt);
    pkt->copy(msg);

    if ((ret = pkt->decode()) != ERROR_SUCCESS) {
        log_error("decode connect response failed. ret=%d", ret);
        return ret;
    }

    DString value;
    if (pkt->findString(StatusCode, value)) {
        if (value == StatusCodeConnectClosed || value == StatusCodeConnectRejected) {
            ret = ERROR_RTMP_CONNECT_REFUSED;
            log_error("connect request is refused. tcUrl=%s, ret=%d", m_req->tcUrl.c_str(), ret);
            return ret;
        }
    }

    if ((ret = send_chunk_size(m_out_chunk_size)) != ERROR_SUCCESS) {
        return ret;
    }

    m_ch->set_out_chunk_size(m_out_chunk_size);

    m_type = CreateStream;

    log_info("connect response finished");

    return ret;
}

int rtmp_client::process_create_stream_response(RtmpMessage *msg)
{
    int ret = ERROR_SUCCESS;

    CreateStreamResPacket *pkt = new CreateStreamResPacket(0,0);
    DAutoFree(CreateStreamResPacket, pkt);
    pkt->copy(msg);

    if ((ret = pkt->decode()) != ERROR_SUCCESS) {
        log_error("decode rtmp createStream response packet failed. ret=%d", ret);
        return ret;
    }

    m_stream_id = pkt->stream_id;

    if (m_publish) {
         m_type = Publish;
    } else {
        m_type = Play;
    }

    return ret;
}

int rtmp_client::process_command_onstatus(RtmpMessage *msg)
{
    int ret = ERROR_SUCCESS;

    OnStatusCallPacket *pkt = new OnStatusCallPacket();
    DAutoFree(OnStatusCallPacket, pkt);
    pkt->copy(msg);

    if ((ret = pkt->decode()) != ERROR_SUCCESS) {
        log_error("decode rtmp onStatus packet failed. ret=%d", ret);
        return ret;
    }

    if (pkt->status_code == StatusCodePublishStart) {
        if (m_publish_start_handler && !m_publish_start_handler(m_req)) {
            ret = ERROR_RTMP_CLIENT_PUBLISH_START;
            return ret;
        }
    }
    if (pkt->status_code == StatusCodeStreamStart) {
        if (m_play_start_handler && !m_play_start_handler(m_req)) {
            ret = ERROR_RTMP_CLIENT_PLAY_START;
            return ret;
        }
    }

    m_type = RecvChunk;

    return ret;
}

int rtmp_client::process_set_chunk_size(RtmpMessage *msg)
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

int rtmp_client::process_window_ackledgement_size(RtmpMessage *msg)
{
    int ret = ERROR_SUCCESS;

    SetWindowAckSizePacket *pkt = new SetWindowAckSizePacket();
    DAutoFree(SetWindowAckSizePacket, pkt);
    pkt->copy(msg);

    if ((ret = pkt->decode()) != ERROR_SUCCESS) {
        log_error("decode rtmp Window_Ackledgement_Size packet failed. ret=%d", ret);
        return ret;
    }

    if (pkt->ackowledgement_window_size > 0) {
        in_ack_size.window = pkt->ackowledgement_window_size;
    }

    return ret;
}

int rtmp_client::process_metadata(RtmpMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if (!m_metadata_handler) {
        return ret;
    }

    OnMetaDataPacket *pkt = new OnMetaDataPacket();
    DAutoFree(OnMetaDataPacket, pkt);
    pkt->copy(msg);

    if ((ret = pkt->decode()) != ERROR_SUCCESS) {
        log_error("decode rtmp metadata packet failed. ret=%d", ret);
        return ret;
    }

    return m_metadata_handler(pkt);
}

int rtmp_client::process_video_audio(RtmpMessage *msg)
{
    if (!m_av_handler) {
        return ERROR_SUCCESS;
    }
    return m_av_handler(msg);
}

int rtmp_client::process_aggregate(RtmpMessage *msg)
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

int rtmp_client::get_command_name_id(char *data, int len, DString &name, double &id)
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
    if (!pkt->findDouble(1, id)) {
        return ERROR_RTMP_TRANSACTION_ID_NOTFOUND;
    }

    return ret;
}

int rtmp_client::get_command_name(char *data, int len, DString &name)
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

int rtmp_client::send_chunk_size(dint32 chunk_size)
{
    int ret = ERROR_SUCCESS;

    SetChunkSizePacket *pkt = new SetChunkSizePacket();
    DAutoFree(SetChunkSizePacket, pkt);
    pkt->chunk_size = chunk_size;

    if ((ret = send_message(pkt)) != ERROR_SUCCESS) {
        log_error("send chunk size packet failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int rtmp_client::send_acknowledgement()
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

int rtmp_client::send_buffer_length(dint64 len)
{
    int ret = ERROR_SUCCESS;

    UserControlPacket *pkt = new UserControlPacket();
    DAutoFree(UserControlPacket, pkt);

    pkt->event_type = SrcPCUCSetBufferLength;
    pkt->extra_data = len;

    if ((ret = send_message(pkt)) != ERROR_SUCCESS) {
        log_error("send set_buffer_length packet failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int rtmp_client::send_message(RtmpMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if ((ret = m_ch->send_message(msg)) != ERROR_SUCCESS) {
        if (ret != SOCKET_EAGAIN) {
            return ret;
        }
    }

    return ERROR_SUCCESS;
}
