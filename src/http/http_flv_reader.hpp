#ifndef HTTP_FLV_READER_HPP
#define HTTP_FLV_READER_HPP

#include "kernel_global.hpp"
#include "http_reader.hpp"

class http_flv_reader
{
public:
    http_flv_reader(http_reader *reader, AVHandler handler);
    ~http_flv_reader();

    int service();

private:
    int read_flv_header();
    int read_tag_header();
    int read_tag_body();
    int read_pre_tag_size();

    void generate_msg(DSharedPtr<MemoryChunk> payload, CommonMessage &msg);

private:
    http_reader *m_reader;
    AVHandler m_av_handler;

    enum Schedule {
        FlvHeader = 0,
        TagHeader,
        TagBody,
        PrevousTagSize
    };
    dint8 m_type;

    duint8 m_tag_type;
    duint32 m_dts;
    duint32 m_tag_len;
};

#endif // HTTP_FLV_READER_HPP
