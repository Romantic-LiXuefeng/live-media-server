#include "rtmp_chunk.hpp"
#include "kernel_errno.hpp"
#include "DStream.hpp"
#include "kernel_log.hpp"
#include "DMemPool.hpp"

rtmp_chunk_read::rtmp_chunk_read(DTcpSocket *socket)
    : m_socket(socket)
    , m_bh_reading(0)
    , m_mh_reading(0)
    , m_p_reading(0)
    , m_fmt(0)
    , m_cid(0)
    , m_entire(false)
    , m_chunk(NULL)
    , m_msg(NULL)
    , m_payload_size(0)
    , m_is_first_chunk_of_msg(true)
    , m_in_chunk_size(128)
{

}

rtmp_chunk_read::~rtmp_chunk_read()
{
    std::map<int, ChunkStream*>::iterator it;

    for (it = m_chunk_streams.begin(); it != m_chunk_streams.end(); ++it) {
        ChunkStream* stream = it->second;
        DFree(stream);
    }

    m_chunk_streams.clear();
}

int rtmp_chunk_read::read()
{
    int ret = ERROR_SUCCESS;

    m_entire = false;

    if ((ret = read_basic_header()) != ERROR_SUCCESS) {
        return ret;
    }

    if (m_bh_reading == 2 && m_mh_reading != 2) {
        if ((ret = read_message_header()) != ERROR_SUCCESS) {
            return ret;
        }
    }

    if (m_mh_reading == 2 && m_p_reading != 2) {
        if ((ret = read_payload()) != ERROR_SUCCESS) {
            return ret;
        }
    }

    if (m_p_reading == 2) {
        reset();
    }

    return ret;
}

CommonMessage *rtmp_chunk_read::getMessage()
{
    return m_msg;
}

bool rtmp_chunk_read::entired()
{
    return m_entire;
}

int rtmp_chunk_read::read_basic_header()
{
    int ret = ERROR_SUCCESS;

    if (!m_bh_reading) {
        if (m_socket->getBufferLen() < 1) {
            return ERROR_KERNEL_BUFFER_NOT_ENOUGH;
        }

        char data[1];
        m_socket->readData(data, 1);

        DStream body(data, 1);
        body.read1Bytes(m_fmt);

        m_cid = m_fmt & 0x3f;
        m_fmt = (m_fmt >> 6) & 0x03;

        if (m_cid > 1) {
            m_bh_reading = 2;

            return ret;
        }

        m_bh_reading++;
    }

    if (m_cid == 0) {
        if (m_bh_reading == 1) {
            if (m_socket->getBufferLen() < 1) {
                return ERROR_KERNEL_BUFFER_NOT_ENOUGH;
            }

            char data[1];
            m_socket->readData(data, 1);

            dint8 cid;
            DStream body(data, 1);
            body.read1Bytes(cid);

            m_cid = 64 + cid;

            m_bh_reading++;
        }
    } else if (m_cid == 1) {
        if (m_bh_reading == 1) {
            if (m_socket->getBufferLen() < 2) {
                return ERROR_KERNEL_BUFFER_NOT_ENOUGH;
            }

            char data[2];
            m_socket->readData(data, 2);

            DStream body(data, 2);

            dint8 cid_1;
            body.read1Bytes(cid_1);
            dint8 cid_2;
            body.read1Bytes(cid_2);

            m_cid = 64 + cid_1 + cid_2 * 256;

            m_bh_reading++;
        }
    }

    return ret;
}

int rtmp_chunk_read::read_message_header()
{
    int ret = ERROR_SUCCESS;

    static char mh_sizes[] = {11, 7, 3, 0};
    int mh_size = mh_sizes[(int)m_fmt];

    bool copy_header = false;

    if (!m_mh_reading) {
        if (mh_size > 0) {
            if (m_socket->getBufferLen() < mh_size) {
                return ERROR_KERNEL_BUFFER_NOT_ENOUGH;
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
            m_chunk->msg = new CommonMessage();
        }

        if (m_fmt <= RTMP_FMT_TYPE2) {
            char data[mh_size];
            m_socket->readData(data, mh_size);

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
            m_mh_reading++;
        } else {
            m_mh_reading = 2;
        }

        copy_header = true;
        m_chunk->first = false;
    }

    if (m_mh_reading == 1) {
        if (m_socket->getBufferLen() < 4) {
            return ERROR_KERNEL_BUFFER_NOT_ENOUGH;
        }

        mh_size += 4;

        char body[4];
        m_socket->copyData(body, 4);

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
            m_socket->readData(data, 4);
        }

        m_mh_reading++;
    }

    if (copy_header) {
        m_chunk->header.timestamp &= 0x7fffffff;
        m_chunk->msg->header = m_chunk->header;
    }

    return ret;
}

int rtmp_chunk_read::read_payload()
{
    int ret = ERROR_SUCCESS;

    if (!m_p_reading) {
        if (m_chunk->header.payload_length <= 0) {
            DFree(m_chunk->msg);
            m_p_reading = 2;
            return ret;
        }

        if (!m_chunk->msg->payload.get()) {
            MemoryChunk *chunk = DMemPool::instance()->getMemory(m_chunk->header.payload_length);
            m_chunk->msg->payload = DSharedPtr<MemoryChunk>(chunk);
        }

        m_payload_size = m_chunk->header.payload_length - m_chunk->msg->payload->length;
        m_payload_size = DMin(m_payload_size, m_in_chunk_size);


        m_p_reading++;
    }

    if (m_p_reading == 1) {
        if (m_socket->getBufferLen() < m_payload_size) {
            return ERROR_KERNEL_BUFFER_NOT_ENOUGH;
        }

        m_socket->readData(m_chunk->msg->payload->data + m_chunk->msg->payload->length, m_payload_size);
        m_chunk->msg->payload->length += m_payload_size;

        if (m_chunk->header.payload_length == m_chunk->msg->payload->length) {
            m_msg = m_chunk->msg;
            m_chunk->msg = NULL;
            m_entire = true;
        }

        m_p_reading++;
    }

    return ret;
}

void rtmp_chunk_read::reset()
{
    m_bh_reading = 0;
    m_mh_reading = 0;
    m_p_reading = 0;
    m_fmt = 0;
    m_cid = 0;
    m_payload_size = 0;
}

/*********************************************************************/

rtmp_chunk_write::rtmp_chunk_write(DTcpSocket *socket)
    : m_socket(socket)
    , m_out_chunk_size(128)
{

}

rtmp_chunk_write::~rtmp_chunk_write()
{

}

int rtmp_chunk_write::send_message(CommonMessage *msg)
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

    return ret;
}

int rtmp_chunk_write::encode_chunk(CommonMessage *msg)
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
            return ERROR_RTMP_CHUNK_HEADER_NEGATIVE;
        } else {
            out_header_cache->length = header_size;
        }

        SendBuffer *header = new SendBuffer;
        header->chunk = out_header_cache;
        header->len = header_size;
        header->pos = 0;

        m_socket->addData(header);

        SendBuffer *body = new SendBuffer;
        body->chunk = msg->payload;
        body->pos = p - msg->payload->data;
        body->len = payload_size;

        m_socket->addData(body);

        // consume sendout bytes when not empty packet.
        if (msg->payload->data && msg->payload->length > 0) {
            p += payload_size;
        }
    } while (p < (char*)msg->payload->data + msg->payload->length);

    return ret;
}
