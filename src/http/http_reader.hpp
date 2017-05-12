#ifndef HTTP_READER_HPP
#define HTTP_READER_HPP

#include "DTcpSocket.hpp"
#include "DMemPool.hpp"

#include <deque>

class http_reader
{
public:
    http_reader(DTcpSocket *socket);
    ~http_reader();

    void set_chunked(bool chunked);

    int grow();

    int read_body(char *data, int len);

    int get_body_len();

private:
    int read_chunk();

private:
    DTcpSocket *m_socket;
    bool m_chunked;

    dint8 m_schedule;

    // the number of bytes of chunk header, 包括length和CRLF
    int m_nb_chunk_header;
    // the number of bytes of current chunk.
    int m_nb_chunk;

    std::deque<MemoryChunk*> m_recv_chunks;
    int m_recv_pos;
    int m_body_len;
};

#endif // HTTP_READER_HPP
