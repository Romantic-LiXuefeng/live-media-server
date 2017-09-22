#include "lms_global.hpp"
#include "kernel_codec.hpp"

RtmpMessage *common_to_rtmp(CommonMessage *msg)
{
    RtmpMessage *ret = new RtmpMessage();
    ret->payload = msg->payload;
    ret->header.payload_length = msg->payload_length;
    ret->header.timestamp = msg->dts;
    ret->header.message_type = msg->type;

    if (msg->type == RTMP_MSG_VideoMessage) {
        ret->header.perfer_cid = RTMP_CID_Video;
    } else if (msg->type == RTMP_MSG_AMF0DataMessage) {
        ret->header.perfer_cid = RTMP_CID_OverConnection2;
    } else if (msg->type == RTMP_MSG_AudioMessage) {
        ret->header.perfer_cid = RTMP_CID_Audio;
    }

    return ret;
}

CommonMessage *rtmp_to_common(RtmpMessage *msg)
{
    CommonMessage *ret = new CommonMessage();
    ret->dts = msg->header.timestamp;
    ret->payload = msg->payload;
    ret->payload_length = msg->payload->length;
    ret->type = msg->header.message_type;

    if (msg->is_video()) {
        if (kernel_codec::video_is_h264(msg->payload->data, msg->payload->length)) {
            dint32 cts = 0;
            DStream stream(msg->payload->data + 2, 3);
            stream.read3Bytes(cts);

            ret->cts = cts;
        }

        if (kernel_codec::video_is_keyframe(msg->payload->data, msg->payload->length)) {
            ret->keyframe = true;
        }

        if (kernel_codec::video_is_sequence_header(msg->payload->data, msg->payload->length)) {
            ret->sequence_header = true;
        }
    }

    if (msg->is_audio()) {
        if (kernel_codec::audio_is_aac(msg->payload->data, msg->payload->length)) {
            if (kernel_codec::audio_is_sequence_header(msg->payload->data, msg->payload->length)) {
                ret->sequence_header = true;
            }
        }
    }

    return ret;
}
