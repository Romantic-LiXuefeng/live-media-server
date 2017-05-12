#ifndef KERNEL_CODEC_HPP
#define KERNEL_CODEC_HPP

#include "DStream.hpp"

// AACPacketType IF SoundFormat == 10 UI8
// The following values are defined:
//     0 = AAC sequence header
//     1 = AAC raw
enum CodecAudioType
{
    // set to the max value to reserved, for array map.
    CodecAudioTypeReserved                        = 2,
    CodecAudioTypeSequenceHeader                 = 0,
    CodecAudioTypeRawData                         = 1
};

// AVCPacketType IF CodecID == 7 UI8
// The following values are defined:
//     0 = AVC sequence header
//     1 = AVC NALU
//     2 = AVC end of sequence (lower level NALU sequence ender is
//         not required or supported)
enum CodecVideoAVCType
{
    // set to the max value to reserved, for array map.
    CodecVideoAVCTypeReserved                    = 3,
    CodecVideoAVCTypeSequenceHeader                 = 0,
    CodecVideoAVCTypeNALU                         = 1,
    CodecVideoAVCTypeSequenceHeaderEOF             = 2
};

// E.4.3.1 VIDEODATA
// Frame Type UB [4]
// Type of video frame. The following values are defined:
//     1 = key frame (for AVC, a seekable frame)
//     2 = inter frame (for AVC, a non-seekable frame)
//     3 = disposable inter frame (H.263 only)
//     4 = generated key frame (reserved for server use only)
//     5 = video info/command frame
enum CodecVideoAVCFrame
{
    // set to the max value to reserved, for array map.
    CodecVideoAVCFrameReserved                    = 0,
    CodecVideoAVCFrameReserved1                    = 6,
    CodecVideoAVCFrameKeyFrame                     = 1,
    CodecVideoAVCFrameInterFrame                 = 2,
    CodecVideoAVCFrameDisposableInterFrame         = 3,
    CodecVideoAVCFrameGeneratedKeyFrame            = 4,
    CodecVideoAVCFrameVideoInfoFrame                = 5
};

// E.4.3.1 VIDEODATA
// CodecID UB [4]
// Codec Identifier. The following values are defined:
//     2 = Sorenson H.263
//     3 = Screen video
//     4 = On2 VP6
//     5 = On2 VP6 with alpha channel
//     6 = Screen video version 2
//     7 = AVC
enum CodecVideo
{
    // set to the max value to reserved, for array map.
    CodecVideoReserved                = 0,
    CodecVideoReserved1                = 1,
    CodecVideoReserved2                = 9,
    // for user to disable video, for example, use pure audio hls.
    CodecVideoDisabled                = 8,
    CodecVideoSorensonH263             = 2,
    CodecVideoScreenVideo             = 3,
    CodecVideoOn2VP6                 = 4,
    CodecVideoOn2VP6WithAlphaChannel = 5,
    CodecVideoScreenVideoVersion2     = 6,
    CodecVideoAVC                     = 7,
    CodecVideoHEVC                     = 10
};

// SoundFormat UB [4]
// Format of SoundData. The following values are defined:
//     0 = Linear PCM, platform endian
//     1 = ADPCM
//     2 = MP3
//     3 = Linear PCM, little endian
//     4 = Nellymoser 16 kHz mono
//     5 = Nellymoser 8 kHz mono
//     6 = Nellymoser
//     7 = G.711 A-law logarithmic PCM
//     8 = G.711 mu-law logarithmic PCM
//     9 = reserved
//     10 = AAC
//     11 = Speex
//     14 = MP3 8 kHz
//     15 = Device-specific sound
// Formats 7, 8, 14, and 15 are reserved.
// AAC is supported in Flash Player 9,0,115,0 and higher.
// Speex is supported in Flash Player 10 and higher.
enum CodecAudio
{
    // set to the max value to reserved, for array map.
    CodecAudioReserved1                = 16,
    // for user to disable audio, for example, use pure video hls.
    CodecAudioDisabled                   = 17,
    CodecAudioLinearPCMPlatformEndian             = 0,
    CodecAudioADPCM                                 = 1,
    CodecAudioMP3                                 = 2,
    CodecAudioLinearPCMLittleEndian                 = 3,
    CodecAudioNellymoser16kHzMono                 = 4,
    CodecAudioNellymoser8kHzMono                 = 5,
    CodecAudioNellymoser                         = 6,
    CodecAudioReservedG711AlawLogarithmicPCM        = 7,
    CodecAudioReservedG711MuLawLogarithmicPCM    = 8,
    CodecAudioReserved                             = 9,
    CodecAudioAAC                                 = 10,
    CodecAudioSpeex                                 = 11,
    CodecAudioReservedMP3_8kHz                     = 14,
    CodecAudioReservedDeviceSpecificSound         = 15
};

class kernel_codec
{
public:
    kernel_codec();
    ~kernel_codec();

public:
    /**
    * only check the frame_type, not check the codec type.
    */
    static bool video_is_keyframe(char* data, int size);
    /**
    * check codec h264, keyframe, sequence header
    */
    static bool video_is_sequence_header(char* data, int size);
    /**
    * check codec aac, sequence header
    */
    static bool audio_is_sequence_header(char* data, int size);
    /**
    * check codec h264.
    */
    static bool video_is_h264(char* data, int size);
    static bool video_is_h265(char* data, int size);
    /**
    * check codec aac.
    */
    static bool audio_is_aac(char* data, int size);
};

#endif // KERNEL_CODEC_HPP
