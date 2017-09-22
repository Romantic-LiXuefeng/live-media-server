#ifndef HTTP_FLV_WRITER_HPP
#define HTTP_FLV_WRITER_HPP

#include "DTcpSocket.hpp"
#include "kernel_global.hpp"
#include "flv_muxer.hpp"

class http_flv_writer
{
public:
    http_flv_writer(DTcpSocket *socket);
    ~http_flv_writer();

    void set_chunked(bool chunked);

    int send_av_data(CommonMessage *msg);
    int send_flv_header();

private:
    DTcpSocket *m_socket;
    flv_muxer *m_muxer;
    bool m_chunked;
};

#endif // HTTP_FLV_WRITER_HPP
