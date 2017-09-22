#include "rtmp_chunk.hpp"
#include "kernel_errno.hpp"
#include "DStream.hpp"
#include "kernel_log.hpp"
#include "DMemPool.hpp"

RtmpChunk::RtmpChunk(DTcpSocket *socket)
    : m_socket(socket)
    , m_fmt(0)
    , m_cid(0)
    , m_entire(false)
    , m_chunk(NULL)
    , m_msg(NULL)
    , m_payload_size(0)
    , m_is_first_chunk_of_msg(true)
    , m_in_chunk_size(128)
    , m_out_chunk_size(128)
    , m_type(BasicLength_1)
{

}

RtmpChunk::~RtmpChunk()
{
    std::map<int, ChunkStream*>::iterator it;
    for (it = m_chunk_streams.begin(); it != m_chunk_streams.end(); ++it) {
        ChunkStream* stream = it->second;
        DFree(stream);
    }
    m_chunk_streams.clear();
}

bool RtmpChunk::entired()
{
    return m_entire;
}

int RtmpChunk::send_message(RtmpMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if ((ret = msg->encode()) != ERROR_SUCCESS) {
        log_error("encode packet failed. ret=%d", ret);
        return ret;
    }

    if ((ret = encode_chunk(msg)) != ERROR_SUCCESS) {
        log_error("encode message to chunk. ret=%d", ret);
        return ret;
    }

    if ((ret = m_socket->flush()) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_RTMP_CHUNK_WRITE, "write rtmp chunk data failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int RtmpChunk::encode_chunk(RtmpMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if (msg->header.perfer_cid < 2) {
        msg->header.perfer_cid = RTMP_CID_ProtocolControl;
    }

    char* p = msg->payload->data;
    char* pp;

    do {
        MemoryChunk *chunk = DMemPool::instance()->getMemory(16);
        DSharedPtr<MemoryChunk> out_header_cache = DSharedPtr<MemoryChunk>(chunk);

        // generate the header.
        char* pheader = out_header_cache->data;

        if (p == msg->payload->data) {
            // write new chunk stream header, fmt is 0
            *pheader++ = 0x00 | (msg->header.perfer_cid & 0x3F);

            // chunk message header, 11 bytes
            // timestamp, 3bytes, big-endian
            duint32 timestamp = (duint32)msg->header.timestamp;
            if (timestamp >= RTMP_EXTENDED_TIMESTAMP) {
                *pheader++ = 0xFF;
                *pheader++ = 0xFF;
                *pheader++ = 0xFF;
            } else {
                pp = (char*)&timestamp;
                *pheader++ = pp[2];
                *pheader++ = pp[1];
                *pheader++ = pp[0];
            }

            // message_length, 3bytes, big-endian
            int payload_length = msg->payload->length;
            pp = (char*)&payload_length;
            *pheader++ = pp[2];
            *pheader++ = pp[1];
            *pheader++ = pp[0];

            // message_type, 1bytes
            *pheader++ = msg->header.message_type;

            // message_length, 3bytes, little-endian
            pp = (char*)&msg->header.stream_id;
            *pheader++ = pp[0];
            *pheader++ = pp[1];
            *pheader++ = pp[2];
            *pheader++ = pp[3];

            // chunk extended timestamp header, 0 or 4 bytes, big-endian
            if(timestamp >= RTMP_EXTENDED_TIMESTAMP){
                pp = (char*)&timestamp;
                *pheader++ = pp[3];
                *pheader++ = pp[2];
                *pheader++ = pp[1];
                *pheader++ = pp[0];
            }
        } else {
            *pheader++ = 0xC0 | (msg->header.perfer_cid & 0x3F);
            duint32 timestamp = (duint32)msg->header.timestamp;
            if(timestamp >= RTMP_EXTENDED_TIMESTAMP){
                pp = (char*)&timestamp;
                *pheader++ = pp[3];
                *pheader++ = pp[2];
                *pheader++ = pp[1];
                *pheader++ = pp[0];
            }
        }

        int payload_size = msg->payload->length - (p - (char*)msg->payload->data);
        payload_size = DMin(payload_size, m_out_chunk_size);

        // always has header
        int header_size = pheader - out_header_cache->data;
        if (header_size <= 0) {
            ret = ERROR_RTMP_CHUNK_HEADER_NEGATIVE;
            log_error("rtmp chunk header size <= 0. ret=%d", ret);
            return ret;
        } else {
            out_header_cache->length = header_size;
        }

        m_socket->add(out_header_cache, header_size, 0);
        m_socket->add(msg->payload, payload_size, p - msg->payload->data);

        // consume sendout bytes when not empty packet.
        if (msg->payload->data && msg->payload->length > 0) {
            p += payload_size;
        }
    } while (p < (char*)msg->payload->data + msg->payload->length);

    return ret;
}

int RtmpChunk::read_chunk()
{
    int ret = ERROR_SUCCESS;

    m_entire = false;

    switch (m_type) {
    case BasicLength_1:
        ret = read_basic_length_1();
        break;
    case BasicLength_2:
        ret = read_basic_length_2();
        break;
    case BasicLength_3:
        ret = read_basic_length_3();
        break;
    case MessageHeader:
        ret = read_message_header();
        break;
    case MessageExtendedTime:
        ret = read_message_header_extended_timestamp();
        break;
    case PayloadInit:
        ret = read_payload_init();
        break;
    case PayloadRead:
        ret = read_payload();
        break;
    case ChunkFinished:
        reset();
        break;
    default:
        break;
    }

    return ret;
}

RtmpMessage *RtmpChunk::get_message()
{
    return m_msg;
}

int RtmpChunk::read_basic_length_1()
{
    int ret = ERROR_SUCCESS;

    char data[1];
    if ((ret = m_socket->read(data, 1)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_CHUNK_READ_BASIC_HEADER, "read chunk basic header length 1 failed. ret=%d", ret);
        return ret;
    }

    DStream body(data, 1);
    body.read1Bytes(m_fmt);

    m_cid = m_fmt & 0x3f;
    m_fmt = (m_fmt >> 6) & 0x03;

    if (m_cid == 0) {
        m_type = BasicLength_2;
    } else if (m_cid == 1) {
        m_type = BasicLength_3;
    } else if (m_cid > 1) {
        m_type = MessageHeader;
    }

    return ret;
}

int RtmpChunk::read_basic_length_2()
{
    int ret = ERROR_SUCCESS;

    char data[1];
    if ((ret = m_socket->read(data, 1)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_CHUNK_READ_BASIC_HEADER, "read chunk basic header length 2 failed. ret=%d", ret);
        return ret;
    }

    dint8 cid;
    DStream body(data, 1);
    body.read1Bytes(cid);

    m_cid = 64 + cid;

    m_type = MessageHeader;

    return ret;
}

int RtmpChunk::read_basic_length_3()
{
    int ret = ERROR_SUCCESS;

    char data[2];
    if ((ret = m_socket->read(data, 2)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_CHUNK_READ_BASIC_HEADER, "read chunk basic header length 3 failed. ret=%d", ret);
        return ret;
    }

    DStream body(data, 2);

    dint8 cid_1;
    body.read1Bytes(cid_1);
    dint8 cid_2;
    body.read1Bytes(cid_2);

    m_cid = 64 + cid_1 + cid_2 * 256;

    m_type = MessageHeader;

    return ret;
}

int RtmpChunk::read_message_header()
{
    int ret = ERROR_SUCCESS;

    static char mh_sizes[] = {11, 7, 3, 0};
    mh_size = mh_sizes[(int)m_fmt];

    m_copy_header = false;

    char data[mh_size];

    if (mh_size > 0 && m_fmt <= RTMP_FMT_TYPE2) {
        if ((ret = m_socket->read(data, mh_size)) != ERROR_SUCCESS) {
            log_error_eagain(ret, ERROR_CHUNK_READ_MESSAGE_HEADER, "read chunk message header failed. ret=%d", ret);
            return ret;
        }
    }

    if (m_chunk_streams.find(m_cid) == m_chunk_streams.end()) {
        m_chunk = m_chunk_streams[m_cid] = new ChunkStream();
        m_chunk->header.perfer_cid = m_cid;
    } else {
        m_chunk = m_chunk_streams[m_cid];
    }

    m_is_first_chunk_of_msg = !m_chunk->msg;

    if (m_chunk->first && m_fmt != RTMP_FMT_TYPE0) {
        if (m_cid != RTMP_CID_ProtocolControl && m_fmt != RTMP_FMT_TYPE1) {
            // support librtmp
            ret = ERROR_RTMP_CHUNK_START;
            log_error("chunk stream is fresh, fmt must be %d, actual is %d. cid=%d, ret=%d",
                      RTMP_FMT_TYPE0, m_fmt, m_cid, ret);

            return ret;
        }
    }

    if (m_chunk->msg && m_fmt == RTMP_FMT_TYPE0) {
        ret = ERROR_RTMP_CHUNK_START;
        log_error("chunk stream exists, fmt must not be %d, actual is %d. ret=%d", RTMP_FMT_TYPE0, m_fmt, ret);
        return ret;
    }

    if (!m_chunk->msg) {
        m_chunk->msg = new RtmpMessage();
    }

    if (m_fmt <= RTMP_FMT_TYPE2) {       
        char *p = data;

        char* pp = (char*)&m_chunk->header.timestamp_delta;
        pp[2] = *p++;
        pp[1] = *p++;
        pp[0] = *p++;
        pp[3] = 0;

        m_chunk->extended_timestamp = (m_chunk->header.timestamp_delta >= RTMP_EXTENDED_TIMESTAMP);

        if (!m_chunk->extended_timestamp) {
            if (m_fmt == RTMP_FMT_TYPE0) {
                m_chunk->header.timestamp = m_chunk->header.timestamp_delta;
            } else {
                m_chunk->header.timestamp += m_chunk->header.timestamp_delta;
            }
        }

        if (m_fmt <= RTMP_FMT_TYPE1) {
            dint32 payload_length = 0;
            pp = (char*)&payload_length;
            pp[2] = *p++;
            pp[1] = *p++;
            pp[0] = *p++;
            pp[3] = 0;

            if (!m_is_first_chunk_of_msg && m_chunk->header.payload_length != payload_length) {
                ret = ERROR_RTMP_PACKET_SIZE;
                log_error("msg exists in chunk cache, size=%d cannot change to %d, ret=%d",
                          m_chunk->header.payload_length, payload_length, ret);

                return ret;
            }

            m_chunk->header.payload_length = payload_length;
            m_chunk->header.message_type = *p++;

            if (m_fmt == RTMP_FMT_TYPE0) {
                pp = (char*)&m_chunk->header.stream_id;
                pp[0] = *p++;
                pp[1] = *p++;
                pp[2] = *p++;
                pp[3] = *p++;
            }
        }
    } else {
        if (m_is_first_chunk_of_msg && !m_chunk->extended_timestamp) {
            m_chunk->header.timestamp += m_chunk->header.timestamp_delta;
        }
    }

    if (m_chunk->extended_timestamp) {
        m_type = MessageExtendedTime;
    } else {
        m_type = PayloadInit;
    }

    m_copy_header = true;
    m_chunk->first = false;

    return ret;
}

int RtmpChunk::read_message_header_extended_timestamp()
{
    int ret = ERROR_SUCCESS;

    char body[4];
    if ((ret = m_socket->copy(body, 4)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_CHUNK_COPY_EXTEND_TIMESTAMP, "copy extend timestamp failed. ret=%d", ret);
        return ret;
    }

    mh_size += 4;

    char *p = body;

    duint32 timestamp = 0x00;
    char* pp = (char*)&timestamp;
    pp[3] = *p++;
    pp[2] = *p++;
    pp[1] = *p++;
    pp[0] = *p++;

    timestamp &= 0x7fffffff;

    duint32 chunk_timestamp = (duint32)m_chunk->header.timestamp;

    if (!m_is_first_chunk_of_msg && chunk_timestamp > 0 && chunk_timestamp != timestamp) {
        mh_size -= 4;
    } else {
        m_chunk->header.timestamp = timestamp;

        char data[4];
        if ((ret = m_socket->read(data, 4)) != ERROR_SUCCESS) {
            ret = ERROR_CHUNK_READ_EXTEND_TIMESTAMP;
            log_error("read extend timestamp failed. ret=%d", ret);
            return ret;
        }
    }

    m_type = PayloadInit;

    return ret;
}

int RtmpChunk::read_payload_init()
{
    int ret = ERROR_SUCCESS;

    if (m_copy_header) {
        m_chunk->header.timestamp &= 0x7fffffff;
        m_chunk->msg->header = m_chunk->header;
    }

    if (m_chunk->header.payload_length <= 0) {
        DFree(m_chunk->msg);
        m_type = ChunkFinished;
        return ret;
    }

    if (!m_chunk->msg->payload.get()) {
        MemoryChunk *chunk = DMemPool::instance()->getMemory(m_chunk->header.payload_length);
        m_chunk->msg->payload = DSharedPtr<MemoryChunk>(chunk);
    }

    m_payload_size = m_chunk->header.payload_length - m_chunk->msg->payload->length;
    m_payload_size = DMin(m_payload_size, m_in_chunk_size);

    m_type = PayloadRead;

    return ret;
}

int RtmpChunk::read_payload()
{
    int ret = ERROR_SUCCESS;

    if ((ret = m_socket->read(m_chunk->msg->payload->data + m_chunk->msg->payload->length, m_payload_size)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_CHUNK_READ_PAYLOAD, "read chunk payload failed. ret=%d", ret);
        return ret;
    }

    m_chunk->msg->payload->length += m_payload_size;

    if (m_chunk->header.payload_length == m_chunk->msg->payload->length) {
        m_msg = m_chunk->msg;
        m_chunk->msg = NULL;
        m_entire = true;
    }

    m_type = ChunkFinished;

    return ret;
}

void RtmpChunk::reset()
{
    m_fmt = 0;
    m_cid = 0;
    m_payload_size = 0;
    m_type = BasicLength_1;
}
