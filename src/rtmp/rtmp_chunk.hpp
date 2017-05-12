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

    CommonMessageHeader header;
    CommonMessage *msg;
};

class rtmp_chunk_read
{
public:
    rtmp_chunk_read(DTcpSocket *socket);
    ~rtmp_chunk_read();

public:
    int read();

    /**
     * 返回的message需要在外部释放
     */
    CommonMessage *getMessage();

    /**
     * 只有为true时才会进行解包处理，否则就继续读chunk
     */
    bool entired();

    void set_in_chunk_size(dint32 in_chunk_size) { m_in_chunk_size = in_chunk_size; }

private:
    int read_basic_header();
    int read_message_header();
    int read_payload();

    void reset();

private:
    DTcpSocket *m_socket;
    std::map<dint32, ChunkStream*> m_chunk_streams;

private:
    // basic header
    duint8 m_bh_reading;
    // message header
    duint8 m_mh_reading;
    // payload
    duint8 m_p_reading;

    dint8 m_fmt;
    dint32 m_cid;
    bool m_entire;

    ChunkStream *m_chunk;
    CommonMessage *m_msg;

    dint32 m_payload_size;

    bool m_is_first_chunk_of_msg;

private:
    dint32 m_in_chunk_size;

};

class rtmp_chunk_write
{
public:
    rtmp_chunk_write(DTcpSocket *socket);
    ~rtmp_chunk_write();

public:
    int send_message(CommonMessage *msg);

    void set_out_chunk_size(dint32 out_chunk_size) { m_out_chunk_size = out_chunk_size; }

private:
    int encode_chunk(CommonMessage *msg);

private:
    DTcpSocket *m_socket;
    dint32 m_out_chunk_size;

};

#endif // RTMP_CHUNK_HPP
