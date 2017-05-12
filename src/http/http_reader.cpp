#include "http_reader.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"
#include <stdlib.h>
#include <string.h>

namespace ChunkReader {
enum chunk_schedule
{
    Header = 0,
    BeginCRLF,
    Body,
    EndCRLF
};
}

http_reader::http_reader(DTcpSocket *socket)
    : m_socket(socket)
    , m_chunked(true)
    , m_schedule(ChunkReader::Header)
    , m_nb_chunk_header(0)
    , m_nb_chunk(0)
    , m_recv_pos(0)
    , m_body_len(0)
{

}

http_reader::~http_reader()
{
    for (int i = 0; i < (int)m_recv_chunks.size(); ++i) {
        DFree(m_recv_chunks.at(i));
    }
    m_recv_chunks.clear();
}

void http_reader::set_chunked(bool chunked)
{
    m_chunked = chunked;
}

int http_reader::grow()
{
    int ret = ERROR_SUCCESS;

    if (m_chunked) {
        while (m_socket->getBufferLen() > 0) {
            if ((ret = read_chunk()) != ERROR_SUCCESS) {
                if (ret == ERROR_KERNEL_BUFFER_NOT_ENOUGH) {
                    break;
                } else if (ret != ERROR_HTTP_READ_EOF) {
                    log_error("http read chunked data failed. ret=%d", ret);
                }
                return ret;
            }
        }
    } else {
        int len = m_socket->getBufferLen();
        if (len <= 0) {
            return ret;
        }

        MemoryChunk *chunk = DMemPool::instance()->getMemory(len);
        if (m_socket->readData(chunk->data, len) < 0) {
            ret = ERROR_TCP_SOCKET_READ_FAILED;
            log_error("http read no chunked data failed. ret=%d", ret);
            DFree(chunk);
            return ret;
        }

        chunk->length = len;
        m_recv_chunks.push_back(chunk);

        m_body_len += len;
    }

    return ERROR_SUCCESS;
}

int http_reader::read_chunk()
{
    int ret = ERROR_SUCCESS;

    if (m_schedule == ChunkReader::Header) {
        int len = m_socket->getBufferLen();

        // 小于3，无法检测'\r\n'，因此不向下进行
        if (len < 3) {
            return ERROR_KERNEL_BUFFER_NOT_ENOUGH;
        }

        MemoryChunk *chunk = DMemPool::instance()->getMemory(len);
        DAutoFree(MemoryChunk, chunk);

        if (m_socket->copyData(chunk->data, len) < 0) {
            ret = ERROR_TCP_SOCKET_COPY_FAILED;
            log_error("http copy chunked header data failed. ret=%d", ret);
            return ret;
        }

        char *start = chunk->data;
        char *end = chunk->data + len;

        for (char *p = start; p < end - 1; p++) {
            if (p[0] == '\r' && p[1] == '\n') {
                if (p == start) {
                    ret = ERROR_HTTP_INVALID_CHUNK_HEADER;
                    log_error("http chunked header start with CRLF. ret=%d", ret);
                    return ret;
                }

                m_nb_chunk_header = (int)(p - start + 2);

                // 没有chunk length，只有'\r\n'
                if (m_nb_chunk_header < 3) {
                    ret = ERROR_HTTP_INVALID_CHUNK_HEADER;
                    log_error("http chunked header length is error. ret=%d", ret);
                    return ret;
                }

                m_schedule = ChunkReader::BeginCRLF;
                break;
            }
        }

        if (m_schedule != ChunkReader::BeginCRLF) {
            return ERROR_KERNEL_BUFFER_NOT_ENOUGH;
        }
    }

    if (m_schedule == ChunkReader::BeginCRLF) {
        MemoryChunk *chunk = DMemPool::instance()->getMemory(m_nb_chunk_header);
        DAutoFree(MemoryChunk, chunk);

        if (m_socket->readData(chunk->data, m_nb_chunk_header) < 0) {
            ret = ERROR_TCP_SOCKET_READ_FAILED;
            log_error("http read chunked header data failed. ret=%d", ret);
            return ret;
        }

        char *at = chunk->data;
        at[m_nb_chunk_header - 1] = 0;
        at[m_nb_chunk_header - 2] = 0;

        int ilength = (int)::strtol(at, NULL, 16);

        if (ilength < 0) {
            ret = ERROR_HTTP_INVALID_CHUNK_HEADER;
            log_error("http chunked header is negatived, length=%d. ret=%d, %d", ilength, ret, m_nb_chunk_header);
            return ret;
        } else if (ilength == 0) {
            // ilength == 0, 视为最后一个chunk
            return ERROR_HTTP_READ_EOF;
        }

        m_nb_chunk = ilength;
        m_schedule = ChunkReader::Body;
    }

    if (m_schedule == ChunkReader::Body) {
        int len = m_socket->getBufferLen();
        if (len < m_nb_chunk) {
            return ERROR_KERNEL_BUFFER_NOT_ENOUGH;
        }

        MemoryChunk *chunk = DMemPool::instance()->getMemory(m_nb_chunk);
        if (m_socket->readData(chunk->data, m_nb_chunk) < 0) {
            ret = ERROR_TCP_SOCKET_READ_FAILED;
            log_error("http read chunked body failed. ret=%d", ret);
            DFree(chunk);
            return ret;
        }

        chunk->length = m_nb_chunk;
        m_recv_chunks.push_back(chunk);

        m_body_len += m_nb_chunk;

        m_schedule = ChunkReader::EndCRLF;
    }

    if (m_schedule == ChunkReader::EndCRLF) {
        if (m_socket->getBufferLen() < 2) {
            return ERROR_KERNEL_BUFFER_NOT_ENOUGH;
        }

        char end[2];
        if (m_socket->readData(end, 2) < 0) {
            ret = ERROR_TCP_SOCKET_READ_FAILED;
            log_error("http read chunked end CRLF failed. ret=%d", ret);
            return ret;
        }

        m_schedule = ChunkReader::Header;
    }

    return ret;
}

int http_reader::read_body(char *data, int len)
{
    int ret = 0;
    int length = len;
    int pos = 0;

    while (1) {
        if (m_recv_chunks.empty() && ret == 0) {
            ret = -1;
            break;
        }

        if (length <= 0) {
            break;
        }

        MemoryChunk *chunk = m_recv_chunks.front();
        int rest = chunk->length - m_recv_pos;

        if (length >= rest) {
            memcpy(data + pos, chunk->data + m_recv_pos, rest);
            length -= rest;
            pos += rest;
            m_recv_pos = 0;
            ret += rest;

            m_body_len -= rest;

            m_recv_chunks.pop_front();
            DFree(chunk);
        } else {
            memcpy(data + pos, chunk->data + m_recv_pos, length);
            m_recv_pos += length;
            ret += length;

            m_body_len -= length;

            break;
        }
    }

    return ret;
}

int http_reader::get_body_len()
{
    return m_body_len;
}

