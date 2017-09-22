#ifndef RTMP_CHUNK_HPP
#define RTMP_CHUNK_HPP

#include "DTcpSocket.hpp"
#include "DGlobal.hpp"
#include "rtmp_global.hpp"

#include <map>

class ChunkStream
{
public:
    ChunkStream()
    {
        extended_timestamp = false;
        msg = NULL;
        first = true;
    }

    ~ChunkStream()
    {
        DFree(msg);
    }

public:
    bool first;

    bool extended_timestamp;

    RtmpMessageHeader header;
    RtmpMessage *msg;
};

class RtmpChunk
{
public:
    RtmpChunk(DTcpSocket *socket);
    ~RtmpChunk();

public:
    int read_chunk();

    /**
     * 返回的message需要在外部释放
     */
    RtmpMessage *get_message();

    /**
     * 只有为true时才会进行解包处理，否则就继续读chunk
     */
    bool entired();

    void set_in_chunk_size(dint32 in_chunk_size) { m_in_chunk_size = in_chunk_size; }
    void set_out_chunk_size(dint32 out_chunk_size) { m_out_chunk_size = out_chunk_size; }

public:
    int send_message(RtmpMessage *msg);

private:
    int encode_chunk(RtmpMessage *msg);

private:
    int read_basic_length_1();
    int read_basic_length_2();
    int read_basic_length_3();

    int read_message_header();
    int read_message_header_extended_timestamp();

    int read_payload_init();
    int read_payload();

    void reset();

private:
    DTcpSocket *m_socket;
    std::map<dint32, ChunkStream*> m_chunk_streams;

private:
    dint8 m_fmt;
    dint32 m_cid;
    bool m_entire;

    ChunkStream *m_chunk;
    RtmpMessage *m_msg;

    dint32 m_payload_size;

    bool m_is_first_chunk_of_msg;

private:
    dint32 m_in_chunk_size;
    dint32 m_out_chunk_size;

    bool m_copy_header;

    int mh_size;

    enum ChunkReadType
    {
        BasicLength_1 = 0,
        BasicLength_2,
        BasicLength_3,
        MessageHeader,
        MessageExtendedTime,
        PayloadInit,
        PayloadRead,
        ChunkFinished
    };
    int8_t m_type;

};

#endif // RTMP_CHUNK_HPP
