#ifndef HTTP_FLV_WRITER_HPP
#define HTTP_FLV_WRITER_HPP

#include "DTcpSocket.hpp"
#include "rtmp_global.hpp"

class http_flv_writer
{
public:
    http_flv_writer(DTcpSocket *socket);
    ~http_flv_writer();

    void set_chunked(bool chunked);

    void send_av_data(CommonMessage *msg);

    void send_flv_header();

private:
    DTcpSocket *m_socket;
    bool m_chunked;
};

#endif // HTTP_FLV_WRITER_HPP
