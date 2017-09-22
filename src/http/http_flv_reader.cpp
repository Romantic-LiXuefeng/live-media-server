#include "http_flv_reader.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"
#include "DStream.hpp"
#include "kernel_codec.hpp"

http_flv_reader::http_flv_reader(http_reader *reader, AVHandler handler)
    : m_reader(reader)
    , m_av_handler(handler)
    , m_type(FlvHeader)
{

}

http_flv_reader::~http_flv_reader()
{
}

int http_flv_reader::read_flv_header()
{
    int ret = ERROR_SUCCESS;

    char header[13];

    if ((ret = m_reader->readBody(header, 13)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_FLV_READ_HEADER, "read http flv header failed. ret=%d", ret);
        return ret;
    }

    if (header[0] != 'F' || header[1] != 'L' || header[2] != 'V') {
        ret = ERROR_FLV_HEADER_TYPE;
        log_error("http flv stream is not start with FLV. ret=%d", ret);
        return ret;
    }

    m_type = TagHeader;

    return ret;
}

int http_flv_reader::read_tag_header()
{
    int ret = ERROR_SUCCESS;

    char header[11];
    if ((ret = m_reader->readBody(header, 11)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_FLV_READ_TAG_HEADER, "read http flv tag header failed. ret=%d", ret);
        return ret;
    }

    // Reserved UB [2]
    // Filter UB [1]
    // TagType UB [5]
    duint8 type = header[0] & 0x1F;

    // DataSize UI24
    duint32 size;
    char *p = (char*)&size;
    p[3] = 0;
    p[2] = header[1];
    p[1] = header[2];
    p[0] = header[3];

    // Timestamp UI24
    duint32 timestamp;
    p = (char*)&timestamp;
    p[2] = header[4];
    p[1] = header[5];
    p[0] = header[6];

    // TimestampExtended UI8
    p[3] = header[7];

    m_tag_type = type;
    m_tag_len = size;
    m_dts = timestamp;

    m_type = TagBody;

    return ret;
}

int http_flv_reader::read_tag_body()
{
    int ret = ERROR_SUCCESS;

    MemoryChunk *chunk = DMemPool::instance()->getMemory(m_tag_len);
    DSharedPtr<MemoryChunk> payload = DSharedPtr<MemoryChunk>(chunk);
    payload->length = m_tag_len;

    if ((ret = m_reader->readBody(payload->data, m_tag_len)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_FLV_READ_TAG_BODY, "http read flv tag body failed. ret=%d", ret);
        return ret;
    }

    m_type = PrevousTagSize;

    CommonMessage msg;
    generate_msg(payload, msg);

    return m_av_handler(&msg);
}

int http_flv_reader::read_pre_tag_size()
{
    int ret = ERROR_SUCCESS;

    char pre_tag[4];
    if ((ret = m_reader->readBody(pre_tag, 4)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_FLV_READ_PREVIOUS_TAG_SIZE, "read http flv previous tag size. ret=%d", ret);
        return ret;
    }

    m_type = TagHeader;

    return ret;
}

void http_flv_reader::generate_msg(DSharedPtr<MemoryChunk> payload, CommonMessage &msg)
{
    msg.type = m_tag_type;
    msg.dts = m_dts;
    msg.payload_length = m_tag_len;
    msg.payload = payload;

    if (msg.is_video()) {
        if (kernel_codec::video_is_h264(payload->data, m_tag_len)) {
            dint32 cts = 0;
            DStream stream(payload->data + 2, 3);
            stream.read3Bytes(cts);

            msg.cts = cts;
        }

        if (kernel_codec::video_is_keyframe(payload->data, m_tag_len)) {
            msg.keyframe = true;
        }

        if (kernel_codec::video_is_sequence_header(payload->data, m_tag_len)) {
            msg.sequence_header = true;
        }
    }

    if (msg.is_audio()) {
        if (kernel_codec::audio_is_aac(payload->data, m_tag_len)) {
            if (kernel_codec::audio_is_sequence_header(payload->data, m_tag_len)) {
                msg.sequence_header = true;
            }
        }
    }
}

int http_flv_reader::service()
{
    int ret = ERROR_SUCCESS;

    while (!m_reader->empty()) {
        switch (m_type) {
        case FlvHeader:
            ret = read_flv_header();
            break;
        case TagHeader:
            ret = read_tag_header();
            break;
        case TagBody:
            ret = read_tag_body();
            break;
        case PrevousTagSize:
            ret = read_pre_tag_size();
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
