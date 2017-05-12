#ifndef HTTP_FLV_READER_HPP
#define HTTP_FLV_READER_HPP

#include "DTcpSocket.hpp"
#include "rtmp_global.hpp"
#include "http_reader.hpp"

class http_flv_reader
{
public:
    http_flv_reader(DTcpSocket *socket);
    ~http_flv_reader();

    void set_chunked(bool chunked);

    int process();

    void set_av_handler(AVHandlerEvent handler);

    bool eof();

private:
    int read_flv_header();
    int read_tag_header();
    int read_tag_body();
    int read_pre_tag_size();

private:
    http_reader *m_reader;
    dint8 m_schedule;
    CommonMessage *m_msg;

    AVHandlerEvent m_av_handler;

    bool m_eof;
};

#endif // HTTP_FLV_READER_HPP
