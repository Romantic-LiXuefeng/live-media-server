#include "flv_muxer.hpp"

flv_muxer::flv_muxer()
{

}

flv_muxer::~flv_muxer()
{

}

DSharedPtr<MemoryChunk> flv_muxer::flv_header()
{
    MemoryChunk *chunk = DMemPool::instance()->getMemory(13);
    DSharedPtr<MemoryChunk> header = DSharedPtr<MemoryChunk>(chunk);

    header->length = 13;

    char *p = header->data;

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

    return header;
}

std::list<DSharedPtr<MemoryChunk> > flv_muxer::encode(CommonMessage *msg)
{
    MemoryChunk *chunk = DMemPool::instance()->getMemory(11);
    DSharedPtr<MemoryChunk> header = DSharedPtr<MemoryChunk>(chunk);

    header->length = 11;

    char *p = header->data;

    // tag header: tag type.
    if (msg->is_video()) {
        *p++ = 0x09;
    } else if (msg->is_audio()) {
        *p++ = 0x08;
    } else if (msg->is_metadata()) {
        *p++ = 0x12;
    }

    // tag header: tag data size.
    char *pp = (char*)&msg->payload_length;
    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];

    // tag header: tag timestamp.
    pp = (char*)&msg->dts;
    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];
    *p++ = pp[3];

    // tag header: stream id always 0.
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x00;

    // 6 = 4 + 2
    // 4 => previous tag size
    // 2 => \r\n
    MemoryChunk *tail_chunk = DMemPool::instance()->getMemory(4);
    DSharedPtr<MemoryChunk> tail = DSharedPtr<MemoryChunk>(tail_chunk);;

    tail->length = 4;

    // previous tag size.
    int pre_tag_len = msg->payload_length + 11;

    p = tail->data;

    pp = (char*)&pre_tag_len;
    *p++ = pp[3];
    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];

    std::list<DSharedPtr<MemoryChunk> > msgs;
    msgs.push_back(header);
    msgs.push_back(msg->payload);
    msgs.push_back(tail);

    return msgs;
}
