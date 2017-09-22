#ifndef HTTP_READER_HPP
#define HTTP_READER_HPP

#include "DTcpSocket.hpp"
#include "DMemPool.hpp"
#include "DHttpParser.hpp"

#include <deque>
#include <tr1/functional>

typedef std::tr1::function<int (DHttpParser*)> HttpHeaderEvent;
#define HTTP_HEADER_CALLBACK(str)  std::tr1::bind(str, this, std::tr1::placeholders::_1)

class http_reader
{
public:
    http_reader(DTcpSocket *socket, bool request, HttpHeaderEvent handler = NULL);
    ~http_reader();

    int service();

    int readBody(char *data, int len);

    int getLength();

    bool empty();

private:
    int read_chunk();
    int read_chunk_header();
    int read_chunk_begin_crlf();
    int read_chunk_body();
    int read_chunk_end_crlf();

    int read_no_chunk();

private:
    int read_http_header();
    int read_http_body();

    int readDataFromBuffer(char *data, int len);

private:
    DTcpSocket *m_socket;
    DHttpParser *m_parser;
    HttpHeaderEvent m_h_handler;

    // the number of bytes of chunk header, 包括length和CRLF
    int m_nb_chunk_header;
    // the number of bytes of current chunk.
    int m_nb_chunk;

    std::deque<MemoryChunk*> m_recv_chunks;
    int m_recv_pos;
    int m_body_len;

    bool m_chunked;

private:
    enum http_schedule
    {
        HttpHeader = 0,
        HttpBody
    };
    dint8 m_h_type;

    enum chunk_schedule
    {
        Header = 0,
        BeginCRLF,
        Body,
        EndCRLF
    };
    dint8 m_type;
};

#endif // HTTP_READER_HPP
