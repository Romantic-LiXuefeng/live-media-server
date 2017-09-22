#ifndef RTMP_GLOBAL_HPP
#define RTMP_GLOBAL_HPP

#include "DMemPool.hpp"
#include "DSharedPtr.hpp"
#include "DStream.hpp"
#include "DGlobal.hpp"
#include "DStringList.hpp"
#include "kernel_request.hpp"
#include "kernel_global.hpp"

#include <tr1/functional>
#include <map>
#include <string.h>

#define RTMP_FMT_TYPE0                          0
#define RTMP_FMT_TYPE1                          1
#define RTMP_FMT_TYPE2                          2
#define RTMP_FMT_TYPE3                          3

#define RTMP_EXTENDED_TIMESTAMP                 0xFFFFFF

#define RTMP_CID_ProtocolControl                0x02
#define RTMP_CID_OverConnection                 0x03
#define RTMP_CID_OverConnection2                0x04
#define RTMP_CID_OverStream                     0x05
#define RTMP_CID_Video                          0x06
#define RTMP_CID_Audio                          0x07
#define RTMP_CID_OverStream2                    0x08

#define RTMP_MSG_SetChunkSize                   0x01
#define RTMP_MSG_AbortMessage                   0x02
#define RTMP_MSG_Acknowledgement                0x03
#define RTMP_MSG_UserControlMessage             0x04
#define RTMP_MSG_WindowAcknowledgementSize      0x05
#define RTMP_MSG_SetPeerBandwidth               0x06
#define RTMP_MSG_AMF3CommandMessage             17 // 0x11
#define RTMP_MSG_AMF0CommandMessage             20 // 0x14
#define RTMP_MSG_AMF0DataMessage                18 // 0x12
#define RTMP_MSG_AMF3DataMessage                15 // 0x0F
#define RTMP_MSG_AudioMessage                   8 // 0x08
#define RTMP_MSG_VideoMessage                   9 // 0x09
#define RTMP_MSG_AggregateMessage               22 // 0x16

#define RTMP_AMF0_COMMAND_CONNECT               "connect"
#define RTMP_AMF0_COMMAND_CREATE_STREAM         "createStream"
#define RTMP_AMF0_COMMAND_CLOSE_STREAM          "closeStream"
#define RTMP_AMF0_COMMAND_PLAY                  "play"
#define RTMP_AMF0_COMMAND_PAUSE                 "pause"
#define RTMP_AMF0_COMMAND_ON_BW_DONE            "onBWDone"
#define RTMP_AMF0_COMMAND_ON_STATUS             "onStatus"
#define RTMP_AMF0_COMMAND_RESULT                "_result"
#define RTMP_AMF0_COMMAND_ERROR                 "_error"
#define RTMP_AMF0_COMMAND_RELEASE_STREAM        "releaseStream"
#define RTMP_AMF0_COMMAND_FC_PUBLISH            "FCPublish"
#define RTMP_AMF0_COMMAND_UNPUBLISH             "FCUnpublish"
#define RTMP_AMF0_COMMAND_PUBLISH               "publish"
#define RTMP_AMF0_DATA_SAMPLE_ACCESS            "|RtmpSampleAccess"
#define RTMP_AMF0_DATA_SET_DATAFRAME            "@setDataFrame"
#define RTMP_AMF0_DATA_ON_METADATA              "onMetaData"

#define RTMP_SIG_FMS_VER                        "3,5,3,888"
#define RTMP_SIG_AMF0_VER                       0
#define RTMP_SIG_CLIENT_ID                      "ASAICiss"

#define StatusLevel                             "level"
#define StatusCode                              "code"
#define StatusDescription                       "description"
#define StatusDetails                           "details"
#define StatusClientId                          "clientid"
// status value
#define StatusLevelStatus                       "status"
// status error
#define StatusLevelError                        "error"
// code value
#define StatusCodeConnectSuccess                "NetConnection.Connect.Success"
#define StatusCodeConnectClosed                 "NetConnection.Connect.Closed"
#define StatusCodeConnectRejected               "NetConnection.Connect.Rejected"
#define StatusCodeStreamReset                   "NetStream.Play.Reset"
#define StatusCodeStreamStart                   "NetStream.Play.Start"
#define StatusCodeStreamPause                   "NetStream.Pause.Notify"
#define StatusCodeStreamUnpause                 "NetStream.Unpause.Notify"
#define StatusCodePublishStart                  "NetStream.Publish.Start"
#define StatusCodeDataStart                     "NetStream.Data.Start"
#define StatusCodeUnpublishSuccess              "NetStream.Unpublish.Success"
#define StatusCodeStreamPublishNotify           "NetStream.Play.PublishNotify"
#define StatusCodeStreamNotFound                "NetStream.Play.StreamNotFound"

// FMLE
#define RTMP_AMF0_COMMAND_ON_FC_PUBLISH         "onFCPublish"
#define RTMP_AMF0_COMMAND_ON_FC_UNPUBLISH       "onFCUnpublish"

#define RTMP_DEFAULT_ACKWINKOW_SIZE (2.5 * 1000 * 1000)

class RtmpMessageHeader
{
public:
    RtmpMessageHeader()
    {
        message_type = 0;
        payload_length = 0;
        timestamp_delta = 0;
        stream_id = 0;
        timestamp = 0;
        perfer_cid = RTMP_CID_OverConnection;
    }

    RtmpMessageHeader(const RtmpMessageHeader &h)
    {
        message_type = h.message_type;
        stream_id = h.stream_id;
        timestamp = h.timestamp;
        payload_length = h.payload_length;
        timestamp_delta = h.timestamp_delta;
        perfer_cid = h.perfer_cid;
    }

    RtmpMessageHeader & operator=(const RtmpMessageHeader &h)
    {
        message_type = h.message_type;
        stream_id = h.stream_id;
        timestamp = h.timestamp;
        payload_length = h.payload_length;
        timestamp_delta = h.timestamp_delta;
        perfer_cid = h.perfer_cid;

        return *this;
    }

public:
    dint8 message_type;
    dint32 stream_id;
    dint64 timestamp;
    dint32 payload_length;
    dint32 timestamp_delta;
    dint32 perfer_cid;
};

class RtmpMessage
{
public:
    RtmpMessage() {}

    RtmpMessage(RtmpMessage *msg)
    {
        header = msg->header;
        payload = msg->payload;
    }

    virtual ~RtmpMessage() {}

    virtual void copy(RtmpMessage *msg)
    {
        this->header = msg->header;
        this->payload = msg->payload;
    }

    virtual int encode() { return 0; }
    virtual int decode() { return 0; }

public:
    bool is_audio() { return header.message_type == RTMP_MSG_AudioMessage; }
    bool is_video() { return header.message_type == RTMP_MSG_VideoMessage; }
    bool is_amf0_command() { return header.message_type == RTMP_MSG_AMF0CommandMessage; }
    bool is_amf0_data() { return header.message_type == RTMP_MSG_AMF0DataMessage; }
    bool is_amf3_command() { return header.message_type == RTMP_MSG_AMF3CommandMessage; }
    bool is_amf3_data() { return header.message_type == RTMP_MSG_AMF3DataMessage; }
    bool is_window_acknowledgement_size() { return header.message_type == RTMP_MSG_WindowAcknowledgementSize; }
    bool is_acknowledgement() { return header.message_type == RTMP_MSG_Acknowledgement; }
    bool is_set_chunk_size() { return header.message_type == RTMP_MSG_SetChunkSize; }
    bool is_user_control_message() { return header.message_type == RTMP_MSG_UserControlMessage; }
    bool is_set_peer_bandwidth() { return header.message_type == RTMP_MSG_SetPeerBandwidth; }
    bool is_aggregate() { return header.message_type == RTMP_MSG_AggregateMessage; }

    void generate_payload(DStream &stream)
    {
        MemoryChunk *chunk = DMemPool::instance()->getMemory(stream.size());
        memcpy(chunk->data, stream.data(), stream.size());
        chunk->length = stream.size();

        payload = DSharedPtr<MemoryChunk>(chunk);
    }

public:
    RtmpMessageHeader header;
    DSharedPtr<MemoryChunk> payload;
};

typedef std::tr1::function<int (RtmpMessage*)> RtmpAVHandler;
#define Rtmp_AV_Handler_Callback(str)  std::tr1::bind(str, this, std::tr1::placeholders::_1)

typedef std::tr1::function<bool (kernel_request*)> RtmpVerifyHandler;
#define Rtmp_Verify_Handler_Callback(str)  std::tr1::bind(str, this, std::tr1::placeholders::_1)

typedef std::tr1::function<void (kernel_request*)> RtmpNotifyHandler;
#define Rtmp_Notify_Handler_Callback(str)  std::tr1::bind(str, this, std::tr1::placeholders::_1)

class AckWindowSize
{
public:
    dint32 window;
    // number of received bytes.
    duint64 nb_recv_bytes;
    // previous responsed sequence number.
    duint64 sequence_number;

    AckWindowSize() : window(0), nb_recv_bytes(0), sequence_number(0){}
};

#endif // RTMP_GLOBAL_HPP
