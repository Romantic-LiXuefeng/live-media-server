#include "kernel_codec.hpp"

kernel_codec::kernel_codec()
{

}

kernel_codec::~kernel_codec()
{

}

bool kernel_codec::video_is_keyframe(char* data, int size)
{
    // 2bytes required.
    if (size < 1) {
        return false;
    }

    char frame_type = data[0];
    frame_type = (frame_type >> 4) & 0x0F;

    return frame_type == CodecVideoAVCFrameKeyFrame;
}

bool kernel_codec::video_is_sequence_header(char* data, int size)
{
    // sequence header only for h264
    if (!video_is_h264(data, size)) {
        return false;
    }

    // 2bytes required.
    if (size < 2) {
        return false;
    }

    char frame_type = data[0];
    frame_type = (frame_type >> 4) & 0x0F;

    char avc_packet_type = data[1];

    return frame_type == CodecVideoAVCFrameKeyFrame
        && avc_packet_type == CodecVideoAVCTypeSequenceHeader;
}

bool kernel_codec::audio_is_sequence_header(char* data, int size)
{
    // sequence header only for aac
    if (!audio_is_aac(data, size)) {
        return false;
    }

    // 2bytes required.
    if (size < 2) {
        return false;
    }

    char aac_packet_type = data[1];

    return aac_packet_type == CodecAudioTypeSequenceHeader;
}

bool kernel_codec::video_is_h264(char* data, int size)
{
    // 1bytes required.
    if (size < 1) {
        return false;
    }

    char codec_id = data[0];
    codec_id = codec_id & 0x0F;

    return codec_id == CodecVideoAVC;
}

bool kernel_codec::video_is_h265(char *data, int size)
{
    // 1bytes required.
    if (size < 1) {
        return false;
    }

    char codec_id = data[0];
    codec_id = codec_id & 0x0F;

    return codec_id == CodecVideoHEVC;
}

bool kernel_codec::audio_is_aac(char* data, int size)
{
    // 1bytes required.
    if (size < 1) {
        return false;
    }

    char sound_format = data[0];
    sound_format = (sound_format >> 4) & 0x0F;

    return sound_format == CodecAudioAAC;
}
