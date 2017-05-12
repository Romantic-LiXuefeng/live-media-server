#include "rtmp_client.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"

rtmp_client::rtmp_client(DTcpSocket *socket)
    : m_socket(socket)
    , m_hs(false)
    , m_ch(false)
    , m_in_chunk_size(128)
    , m_out_chunk_size(128)
    , m_window_ack_size(2500000)
    , m_window_response_size(0)
    , m_window_last_ack(0)
    , m_window_total_size(0)
    , m_player_buffer_length(3000)
    , m_stream_id(1)
    , m_can_publish(false)
    , m_timeout(30 * 1000 * 1000)
    , m_schedule(RtmpClientSchedule::Default)
    , m_publish(false)
    , m_av_handler(NULL)
    , m_metadata_handler(NULL)
{
    m_hs_handler = new rtmp_handshake(m_socket);
    m_chunk_reader = new rtmp_chunk_read(m_socket);
    m_chunk_writer = new rtmp_chunk_write(m_socket);
    m_req = new rtmp_request();
}

rtmp_client::~rtmp_client()
{
    DFree(m_hs_handler);
    DFree(m_chunk_reader);
    DFree(m_chunk_writer);
    DFree(m_req);
}

int rtmp_client::do_process()
{
    int ret = ERROR_SUCCESS;

    while (m_socket->getBufferLen() > 0) {
        if ((ret = handshake_and_connect()) != ERROR_SUCCESS) {
            if (ret == ERROR_KERNEL_BUFFER_NOT_ENOUGH) {
                break;
            }
            log_error("rtmp handshake or connect failed. ret=%d", ret);
            return ret;
        }

        if ((ret = read_chunk()) != ERROR_SUCCESS) {
            if (ret == ERROR_KERNEL_BUFFER_NOT_ENOUGH) {
                break;
            }
            log_error("read rtmp chunk failed. ret=%d", ret);
            return ret;
        }

        if ((ret = decode_message()) != ERROR_SUCCESS) {
            log_error("decode and process rtmp message failed. ret=%d", ret);
            return ret;
        }
    }

    return ERROR_SUCCESS;
}

void rtmp_client::set_chunk_size(dint32 chunk_size)
{
    m_out_chunk_size = chunk_size;
}

void rtmp_client::set_buffer_length(dint64 len)
{
    m_player_buffer_length = len;
}

void rtmp_client::set_connect_packet(DSharedPtr<ConnectAppPacket> pkt)
{
    m_conn_pkt = pkt;
}

int rtmp_client::start_rtmp(bool publish, rtmp_request *req)
{
    int ret = ERROR_SUCCESS;

    m_publish = publish;
    m_req->copy(req);

    if ((ret = handshake_and_connect()) != ERROR_SUCCESS) {
        if (ret != ERROR_KERNEL_BUFFER_NOT_ENOUGH) {
            log_error("rtmp handshake or connect failed. ret=%d", ret);
            return ret;
        }
    }

    return ERROR_SUCCESS;
}

int rtmp_client::stop_rtmp()
{
    int ret = ERROR_SUCCESS;

    CloseStreamPacket *pkt = new CloseStreamPacket();
    DAutoFree(CloseStreamPacket, pkt);

    if ((ret = m_chunk_writer->send_message(pkt)) != ERROR_SUCCESS) {
        log_error("send rtmp closeStream packet failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int rtmp_client::send_av_data(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    msg->header.stream_id = m_stream_id;

    if ((ret = m_chunk_writer->send_message(msg)) != ERROR_SUCCESS) {
        log_error("send audio/video message to server failed. type=%d, ret=%d", msg->header.message_type, ret);
        return ret;
    }

    return ret;
}

void rtmp_client::set_av_handler(AVHandlerEvent handler)
{
    m_av_handler = handler;
}

void rtmp_client::set_metadata_handler(AVHandlerEvent handler)
{
    m_metadata_handler = handler;
}

bool rtmp_client::can_publish()
{
    return m_can_publish;
}

void rtmp_client::set_timeout(dint64 timeout)
{
    m_timeout = timeout;
}

int rtmp_client::read_chunk()
{
    int ret = ERROR_SUCCESS;

    if (!m_hs)
        return ret;

    if ((ret = m_chunk_reader->read()) != ERROR_SUCCESS) {
        if (ret != ERROR_KERNEL_BUFFER_NOT_ENOUGH) {
            log_error("read rtmp chunk failed. ret=%d", ret);
        }
        return ret;
    }

    if (m_chunk_reader->entired()) {
        m_ch = true;
    }

    return ret;
}

int rtmp_client::decode_message()
{
    int ret = ERROR_SUCCESS;

    if (!m_ch) {
        return ret;
    }
    m_ch = false;

    CommonMessage *msg = m_chunk_reader->getMessage();
    DAutoFree(CommonMessage, msg);

    if (true) {
        duint64 total_recv_size = m_socket->get_recv_size();
        duint64 delta = total_recv_size - m_window_total_size;
        m_window_total_size = total_recv_size;
        m_window_response_size += delta;

        if (m_window_response_size >= 0xf0000000) {
            m_window_response_size = 0;
            m_window_last_ack = 0;
        }

        if (m_window_ack_size > 0 && ((m_window_response_size - m_window_last_ack) >= (duint64)m_window_ack_size)) {
            m_window_last_ack = m_window_response_size;
            if ((ret = send_acknowledgement((dint32)m_window_response_size)) != ERROR_SUCCESS) {
                log_error("send rtmp acknowledgement packet failed. ret=%d", ret);
                return ret;
            }
        }
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

int rtmp_client::handshake_and_connect()
{
    int ret = ERROR_SUCCESS;

    if (m_hs) {
        return ret;
    }

    if ((ret = m_hs_handler->handshake_with_server(false)) != ERROR_SUCCESS) {
        if (ret != ERROR_KERNEL_BUFFER_NOT_ENOUGH) {
            log_error("rtmp handshake with server failed. ret=%d", ret);
            DFree(m_hs_handler);
        }
        return ret;
    }

    m_hs = true;
    DFree(m_hs_handler);

    // 握手完毕之后，需要发送connect包，放在这里触发可以减少判断，不必记录connect的状态
    if((ret = connect_app()) != ERROR_SUCCESS) {
        log_error("send rtmp connect packet failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int rtmp_client::connect_app()
{
    int ret = ERROR_SUCCESS;

    if (m_schedule != RtmpClientSchedule::Default) {
        return ret;
    }

    if (!m_conn_pkt.get()) {
        ConnectAppPacket *pkt = new ConnectAppPacket();
        m_conn_pkt = DSharedPtr<ConnectAppPacket>(pkt);

        m_conn_pkt->command_object.setValue("app", new AMF0String(m_req->app));
        m_conn_pkt->command_object.setValue("flashVer", new AMF0String("WIN 12,0,0,41"));
        m_conn_pkt->command_object.setValue("swfUrl", new AMF0String(m_req->swfUrl));
        m_conn_pkt->command_object.setValue("tcUrl", new AMF0String(m_req->tcUrl));
        m_conn_pkt->command_object.setValue("fpad", new AMF0Boolean(false));
        m_conn_pkt->command_object.setValue("capabilities", new AMF0Number(239));
        m_conn_pkt->command_object.setValue("audioCodecs", new AMF0Number(3575));
        m_conn_pkt->command_object.setValue("videoCodecs", new AMF0Number(252));
        m_conn_pkt->command_object.setValue("videoFunction", new AMF0Number(1));
        m_conn_pkt->command_object.setValue("objectEncoding", new AMF0Number(0));
    }

    if ((ret = m_chunk_writer->send_message(m_conn_pkt.get())) != ERROR_SUCCESS) {
        return ret;
    }

    m_requests[m_conn_pkt->transaction_id] = m_conn_pkt->command_name;
    m_schedule = RtmpClientSchedule::Connect;

    return ret;
}

int rtmp_client::create_stream()
{
    int ret = ERROR_SUCCESS;

    if (m_schedule != RtmpClientSchedule::ConnectResponsed) {
        return ret;
    }

    CreateStreamPacket *pkt = new CreateStreamPacket();
    DAutoFree(CreateStreamPacket, pkt);

    if ((ret = m_chunk_writer->send_message(pkt)) != ERROR_SUCCESS) {
        return ret;
    }

    m_requests[pkt->transaction_id] = pkt->command_name;
    m_schedule = RtmpClientSchedule::CreateStream;

    return ret;
}

int rtmp_client::publish()
{
    int ret = ERROR_SUCCESS;

    if (m_schedule != RtmpClientSchedule::CreateStreamResponsed) {
        return ret;
    }

    PublishPacket *pkt = new PublishPacket();
    DAutoFree(PublishPacket, pkt);

    pkt->header.stream_id = m_stream_id;
    pkt->stream_name = m_req->stream;

    if ((ret = m_chunk_writer->send_message(pkt)) != ERROR_SUCCESS) {
        log_error("send rtmp publish packet failed. ret=%d", ret);
        return ret;
    }

    m_schedule = RtmpClientSchedule::Publish;

    log_trace("start publish");

    return ret;
}

int rtmp_client::play()
{
    int ret = ERROR_SUCCESS;

    if (m_schedule != RtmpClientSchedule::CreateStreamResponsed) {
        return ret;
    }

    if ((ret = send_buffer_length(m_player_buffer_length)) != ERROR_SUCCESS) {
        log_error("send rtmp set_buffer_length packet failed. ret=%d", ret);
        return ret;
    }

    PlayPacket *pkt = new PlayPacket();
    DAutoFree(PlayPacket, pkt);

    pkt->header.stream_id = m_stream_id;
    pkt->stream_name = m_req->stream;

    if ((ret = m_chunk_writer->send_message(pkt)) != ERROR_SUCCESS) {
        log_error("send rtmp play packet failed. ret=%d", ret);
        return ret;
    }

    m_schedule = RtmpClientSchedule::play;

    log_trace("start play");

    return ret;
}

int rtmp_client::process_connect_response(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if (m_schedule != RtmpClientSchedule::Connect) {
        return ret;
    }

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

    m_schedule = RtmpClientSchedule::ConnectResponsed;

    if ((ret = send_chunk_size(m_out_chunk_size)) != ERROR_SUCCESS) {
        log_error("send rtmp set_chunk_size packet failed. ret=%d", ret);
        return ret;
    }

    m_chunk_writer->set_out_chunk_size(m_out_chunk_size);

    if ((ret = create_stream()) != ERROR_SUCCESS) {
        log_error("send rtmp createStream packet failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int rtmp_client::process_create_stream_response(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if (m_schedule != RtmpClientSchedule::CreateStream) {
        return ret;
    }

    CreateStreamResPacket *pkt = new CreateStreamResPacket(0,0);
    DAutoFree(CreateStreamResPacket, pkt);
    pkt->copy(msg);

    if ((ret = pkt->decode()) != ERROR_SUCCESS) {
        log_error("decode rtmp createStream response packet failed. ret=%d", ret);
        return ret;
    }

    m_stream_id = pkt->stream_id;
    m_schedule = RtmpClientSchedule::CreateStreamResponsed;

    if (m_publish) {
        if((ret = publish()) != ERROR_SUCCESS) {
            return ret;
        }
    } else {
        if((ret = play()) != ERROR_SUCCESS) {
            return ret;
        }
    }

    return ret;
}

int rtmp_client::process_command_onstatus(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if (m_schedule != RtmpClientSchedule::Publish && m_schedule != RtmpClientSchedule::play) {
        return ret;
    }

    OnStatusCallPacket *pkt = new OnStatusCallPacket();
    DAutoFree(OnStatusCallPacket, pkt);
    pkt->copy(msg);

    if ((ret = pkt->decode()) != ERROR_SUCCESS) {
        log_error("decode rtmp onStatus packet failed. ret=%d", ret);
        return ret;
    }

    if (pkt->status_code == StatusCodePublishStart) {
        m_schedule = RtmpClientSchedule::PublishResponsed;
        m_can_publish = true;

        m_socket->setRecvTimeOut(-1);
        m_socket->setSendTimeOut(m_timeout);
    }
    if (pkt->status_code == StatusCodeStreamStart) {
        m_schedule = RtmpClientSchedule::playResponsed;

        m_socket->setSendTimeOut(-1);
        m_socket->setRecvTimeOut(m_timeout);
    }

    return ret;
}

int rtmp_client::process_set_chunk_size(CommonMessage *msg)
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
    m_chunk_reader->set_in_chunk_size(m_in_chunk_size);

    log_trace("peer_chunk_size=%d", m_in_chunk_size);
    return ret;
}

int rtmp_client::process_window_ackledgement_size(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    SetWindowAckSizePacket *pkt = new SetWindowAckSizePacket();
    DAutoFree(SetWindowAckSizePacket, pkt);
    pkt->copy(msg);

    if ((ret = pkt->decode()) != ERROR_SUCCESS) {
        log_error("decode rtmp Window_Ackledgement_Size packet failed. ret=%d", ret);
        return ret;
    }

    m_window_ack_size = pkt->ackowledgement_window_size;

    return ret;
}

int rtmp_client::process_metadata(CommonMessage *msg)
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
        DFree(pkt);
        return ret;
    }

    return m_metadata_handler(pkt);
}

int rtmp_client::process_video_audio(CommonMessage *msg)
{
    if (!m_av_handler) {
        return ERROR_SUCCESS;
    }
    return m_av_handler(msg);
}

int rtmp_client::process_aggregate(CommonMessage *msg)
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

        CommonMessage _msg;
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

    if ((ret = m_chunk_writer->send_message(pkt)) != ERROR_SUCCESS) {
        log_error("write chunk size message failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int rtmp_client::send_acknowledgement(dint32 recv_size)
{
    int ret = ERROR_SUCCESS;

    AcknowledgementPacket *pkt = new AcknowledgementPacket();
    DAutoFree(AcknowledgementPacket, pkt);
    pkt->sequence_number = recv_size;

    if ((ret = m_chunk_writer->send_message(pkt)) != ERROR_SUCCESS) {
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

    if ((ret = m_chunk_writer->send_message(pkt)) != ERROR_SUCCESS) {
        log_error("write set buffer length message failed. ret=%d", ret);
        return ret;
    }

    return ret;
}
