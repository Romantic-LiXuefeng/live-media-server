#include "http_flv_reader.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"

namespace FlvReader {
    enum Schedule {
        flv_header = 0,
        tag_header,
        tag_body,
        prevous_tag_size
    };
}

http_flv_reader::http_flv_reader(DTcpSocket *socket)
    : m_schedule(FlvReader::flv_header)
    , m_msg(NULL)
    , m_av_handler(NULL)
    , m_eof(false)
{
    m_reader = new http_reader(socket);
}

http_flv_reader::~http_flv_reader()
{
    DFree(m_reader);
    DFree(m_msg);
}

void http_flv_reader::set_chunked(bool chunked)
{
    m_reader->set_chunked(chunked);
}

int http_flv_reader::process()
{
    int ret = ERROR_SUCCESS;

    if ((ret = m_reader->grow()) != ERROR_SUCCESS) {
        if (ret == ERROR_HTTP_READ_EOF) {
            m_eof = true;
            log_trace("http chunked data completed.");
            return ERROR_SUCCESS;
        }
        log_error("reader grow failed. ret=%d", ret);
        return ret;
    }

    while (m_reader->get_body_len() > 0) {
        if ((ret = read_flv_header()) != ERROR_SUCCESS) {
            if (ret == ERROR_KERNEL_BUFFER_NOT_ENOUGH) {
                break;
            }
            return ret;
        }

        if ((ret = read_tag_header()) != ERROR_SUCCESS) {
            if (ret == ERROR_KERNEL_BUFFER_NOT_ENOUGH) {
                break;
            }
            return ret;
        }

        if ((ret = read_tag_body()) != ERROR_SUCCESS) {
            if (ret == ERROR_KERNEL_BUFFER_NOT_ENOUGH) {
                break;
            }
            return ret;
        }

        if ((ret = read_pre_tag_size()) != ERROR_SUCCESS) {
            if (ret == ERROR_KERNEL_BUFFER_NOT_ENOUGH) {
                break;
            }
            return ret;
        }
    }

    return ERROR_SUCCESS;
}

void http_flv_reader::set_av_handler(AVHandlerEvent handler)
{
    m_av_handler = handler;
}

bool http_flv_reader::eof()
{
    return m_eof;
}

int http_flv_reader::read_flv_header()
{
    int ret = ERROR_SUCCESS;

    if (m_schedule != FlvReader::flv_header) {
        return ret;
    }

    int len = m_reader->get_body_len();
    if (len < 13) {
        return ERROR_KERNEL_BUFFER_NOT_ENOUGH;
    }

    char header[13];

    if (m_reader->read_body(header, 13) < 0) {
        ret = ERROR_FLV_READ_HEADER;
        log_error("read http flv header failed. ret=%d", ret);
        return ret;
    }

    if (header[0] != 'F' || header[1] != 'L' || header[2] != 'V') {
        ret = ERROR_FLV_HEADER_TYPE;
        log_error("http flv is not start with FLV. ret=%d", ret);
        return ret;
    }

    m_schedule = FlvReader::tag_header;

    return ret;
}

int http_flv_reader::read_tag_header()
{
    int ret = ERROR_SUCCESS;

    if (m_schedule != FlvReader::tag_header) {
        return ret;
    }

    int len = m_reader->get_body_len();
    if (len < 11) {
        return ERROR_KERNEL_BUFFER_NOT_ENOUGH;
    }

    char header[11];
    if (m_reader->read_body(header, 11) < 0) {
        ret = ERROR_FLV_READ_TAG_HEADER;
        log_error("read http flv tag header failed. ret=%d", ret);
        return ret;
    }

    // Reserved UB [2]
    // Filter UB [1]
    // TagType UB [5]
    dint8 type = header[0] & 0x1F;

    // DataSize UI24
    dint32 size;
    char *p = (char*)&size;
    p[3] = 0;
    p[2] = header[1];
    p[1] = header[2];
    p[0] = header[3];

    // Timestamp UI24
    dint32 timestamp;
    p = (char*)&timestamp;
    p[2] = header[4];
    p[1] = header[5];
    p[0] = header[6];

    // TimestampExtended UI8
    p[3] = header[7];

    DFree(m_msg);
    m_msg = new CommonMessage();
    m_msg->header.message_type = type;
    m_msg->header.stream_id = 0;
    m_msg->header.timestamp = timestamp;
    m_msg->header.payload_length = size;

    if (type == RTMP_MSG_VideoMessage) {
        m_msg->header.perfer_cid = RTMP_CID_Video;
    } else if (type == RTMP_MSG_AMF0DataMessage) {
        m_msg->header.perfer_cid = RTMP_CID_OverConnection2;
    } else if (type == RTMP_MSG_AudioMessage) {
        m_msg->header.perfer_cid = RTMP_CID_Audio;
    }

    m_schedule = FlvReader::tag_body;

    return ret;
}

int http_flv_reader::read_tag_body()
{
    int ret = ERROR_SUCCESS;

    if (m_schedule != FlvReader::tag_body) {
        return ret;
    }

    int len = m_reader->get_body_len();
    if (len < m_msg->header.payload_length) {
        return ERROR_KERNEL_BUFFER_NOT_ENOUGH;
    }

    MemoryChunk *chunk = DMemPool::instance()->getMemory(m_msg->header.payload_length);
    if (m_reader->read_body(chunk->data, m_msg->header.payload_length) < 0) {
        ret = ERROR_FLV_READ_TAG_BODY;
        log_error("http read flv tag body failed. ret=%d", ret);
        DFree(chunk);
        return ret;
    }
    m_msg->payload = DSharedPtr<MemoryChunk>(chunk);
    m_msg->payload->length = m_msg->header.payload_length;

    m_schedule = FlvReader::prevous_tag_size;

    if (m_av_handler) {
        return m_av_handler(m_msg);
    }

    return ret;
}

int http_flv_reader::read_pre_tag_size()
{
    int ret = ERROR_SUCCESS;

    if (m_schedule != FlvReader::prevous_tag_size) {
        return ret;
    }

    int len = m_reader->get_body_len();
    if (len < 4) {
        return ERROR_KERNEL_BUFFER_NOT_ENOUGH;
    }

    char pre_tag[4];
    if (m_reader->read_body(pre_tag, 4) < 0) {
        ret = ERROR_FLV_READ_PREVIOUS_TAG_SIZE;
        log_error("read http flv previous tag size. ret=%d", ret);
        return ret;
    }

    m_schedule = FlvReader::tag_header;

    return ret;
}
