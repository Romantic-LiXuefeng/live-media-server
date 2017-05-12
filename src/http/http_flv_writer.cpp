#include "http_flv_writer.hpp"
#include "DSharedPtr.hpp"
#include "DMemPool.hpp"

http_flv_writer::http_flv_writer(DTcpSocket *socket)
    : m_socket(socket)
    , m_chunked(true)
{

}

http_flv_writer::~http_flv_writer()
{

}

void http_flv_writer::set_chunked(bool chunked)
{
    m_chunked = chunked;
}

void http_flv_writer::send_av_data(CommonMessage *msg)
{
    int nb_size = 0;

    // 64 => chunk size
    // 2  => \r\n
    // 11 => tag header size
    // 4  => previous tag size
    MemoryChunk *chunk = NULL;
    DSharedPtr<MemoryChunk> header;

    if (m_chunked) {
        chunk = DMemPool::instance()->getMemory(64 + 2 + 11);
    } else {
        chunk = DMemPool::instance()->getMemory(64 + 11);
    }

    header = DSharedPtr<MemoryChunk>(chunk);
    int chunked_size = 11 + msg->payload->length + 4;

    if (m_chunked) {
        nb_size = snprintf(header->data, 64, "%x", chunked_size);
    }

    header->length = nb_size + 2 + 11;

    char *p = header->data + nb_size;

    if (m_chunked) {
        *p++ = '\r';
        *p++ = '\n';
    }

    // tag header: tag type.
    if (msg->is_video()) {
        *p++ = 0x09;
    } else if (msg->is_audio()) {
        *p++ = 0x08;
    } else {
        *p++ = 0x12;
    }

    // tag header: tag data size.
    char *pp = (char*)&msg->payload->length;
    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];

    // tag header: tag timestamp.
    pp = (char*)&msg->header.timestamp;
    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];
    *p++ = pp[3];

    // tag header: stream id always 0.
    pp = (char*)&msg->header.stream_id;
    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];

    SendBuffer *buf = new SendBuffer;
    buf->chunk = header;
    buf->len = header->length;
    buf->pos = 0;

    m_socket->addData(buf);

    SendBuffer *body = new SendBuffer;
    body->chunk = msg->payload;
    body->len = msg->payload->length;
    body->pos = 0;

    m_socket->addData(body);

    // 6 = 4 + 2
    // 4 => previous tag size
    // 2 => \r\n
    MemoryChunk *pre_chunk = NULL;
    DSharedPtr<MemoryChunk> tail;

    if (m_chunked) {
        pre_chunk = DMemPool::instance()->getMemory(6);
        tail = DSharedPtr<MemoryChunk>(pre_chunk);
        tail->length = 6;
    } else {
        pre_chunk = DMemPool::instance()->getMemory(4);
        tail = DSharedPtr<MemoryChunk>(pre_chunk);
        tail->length = 4;
    }

    p = tail->data;

    // previous tag size.
    int pre_tag_len = msg->payload->length + 11;

    pp = (char*)&pre_tag_len;
    *p++ = pp[3];
    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];

    if (m_chunked) {
        *p++ = '\r';
        *p++ = '\n';
    }

    SendBuffer *pre_tag = new SendBuffer;
    pre_tag->chunk = tail;
    pre_tag->len = tail->length;
    pre_tag->pos = 0;

    m_socket->addData(pre_tag);
}

void http_flv_writer::send_flv_header()
{
    int nb_size = 0;

    // 81 = 64 + 2 + 13 + 2
    MemoryChunk *chunk = DMemPool::instance()->getMemory(81);
    DSharedPtr<MemoryChunk> header = DSharedPtr<MemoryChunk>(chunk);

    if (m_chunked) {
        nb_size = snprintf(header->data, 64, "%x", 13);
    }

    char *p = header->data + nb_size;

    if (m_chunked) {
        *p++ = '\r';
        *p++ = '\n';
    }

    // flv header
    *p++ = 'F';
    *p++ = 'L';
    *p++ = 'V';
    *p++ = 0x01;
    *p++ = 0x05;
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x09;
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x00;

    if (m_chunked) {
        *p++ = '\r';
        *p++ = '\n';
    }

    if (m_chunked) {
        header->length = nb_size + 2 + 13 + 2;
    } else {
        header->length = 13;
    }

    SendBuffer *buf = new SendBuffer;
    buf->chunk = header;
    buf->len = header->length;
    buf->pos = 0;

    m_socket->addData(buf);
}
