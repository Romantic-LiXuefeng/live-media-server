#include "http_reader.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"
#include <stdlib.h>
#include <string.h>

http_reader::http_reader(DTcpSocket *socket, bool request, HttpHeaderEvent handler)
    : m_socket(socket)
    , m_h_handler(handler)
    , m_nb_chunk_header(0)
    , m_nb_chunk(0)
    , m_recv_pos(0)
    , m_body_len(0)
    , m_chunked(true)
    , m_h_type(HttpHeader)
    , m_type(Header)
{
    enum http_parser_type type = (request == true) ? HTTP_REQUEST : HTTP_RESPONSE;
    m_parser = new DHttpParser(type);
}

http_reader::~http_reader()
{
    for (int i = 0; i < (int)m_recv_chunks.size(); ++i) {
        DFree(m_recv_chunks.at(i));
    }
    m_recv_chunks.clear();

    DFree(m_parser);
}

int http_reader::service()
{
    int ret = ERROR_SUCCESS;

    while (true) {
        switch (m_h_type) {
        case HttpHeader:
            ret = read_http_header();
            break;
        case HttpBody:
            ret = read_http_body();
            break;
        default:
            break;
        }

        if (ret != ERROR_SUCCESS) {
            break;
        }
    }

    return ret;
}

int http_reader::read_chunk()
{
    int ret = ERROR_SUCCESS;

    switch (m_type) {
    case Header:
        ret = read_chunk_header();
        break;
    case BeginCRLF:
        ret = read_chunk_begin_crlf();
        break;
    case Body:
        ret = read_chunk_body();
        break;
    case EndCRLF:
        ret = read_chunk_end_crlf();
        break;
    default:
        break;
    }

    return ret;
}

int http_reader::read_chunk_header()
{
    int ret = ERROR_SUCCESS;

    int len = m_socket->getReadBufferLength();
    // 小于3，无法检测'\r\n'，因此不向下进行
    if (len < 3) {
        return SOCKET_EAGAIN;
    }

    MemoryChunk *chunk = DMemPool::instance()->getMemory(len);
    DAutoFree(MemoryChunk, chunk);

    if ((ret = m_socket->copy(chunk->data, len)) != ERROR_SUCCESS) {
        ret = ERROR_HTTP_COPY_CHUNK_HEADER;
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

            m_type = BeginCRLF;
            break;
        }
    }

    if (m_type != BeginCRLF) {
        return SOCKET_EAGAIN;
    }

    return ret;
}

int http_reader::read_chunk_begin_crlf()
{
    int ret = ERROR_SUCCESS;

    MemoryChunk *chunk = DMemPool::instance()->getMemory(m_nb_chunk_header);
    chunk->length = m_nb_chunk_header;
    DAutoFree(MemoryChunk, chunk);

    if ((ret = m_socket->read(chunk->data, m_nb_chunk_header)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_HTTP_READ_CHUNK_BEGIN_CRLF, "http read chunked header data failed. ret=%d", ret);
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
    }

    m_nb_chunk = ilength;
    m_type = Body;

    return ret;
}

int http_reader::read_chunk_body()
{
    int ret = ERROR_SUCCESS;

    MemoryChunk *chunk = DMemPool::instance()->getMemory(m_nb_chunk);
    chunk->length = m_nb_chunk;

    if ((ret = m_socket->read(chunk->data, m_nb_chunk)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_HTTP_READ_CHUNK_BODY, "http read chunked body failed. ret=%d", ret);
        DFree(chunk);
        return ret;
    }

    m_recv_chunks.push_back(chunk);

    m_body_len += m_nb_chunk;

    m_type = EndCRLF;

    return ret;
}

int http_reader::read_chunk_end_crlf()
{
    int ret = ERROR_SUCCESS;

    char end[2];

    if ((ret = m_socket->read(end, 2)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_HTTP_READ_CHUNK_END_CRLF, "http read chunked end CRLF failed. ret=%d", ret);
        return ret;
    }

    m_type = Header;

    return ret;
}

int http_reader::read_no_chunk()
{
    int ret = ERROR_SUCCESS;

    int len = m_socket->getReadBufferLength();
    if (len <= 0) {
        return SOCKET_EAGAIN;
    }

    MemoryChunk *chunk = DMemPool::instance()->getMemory(len);
    chunk->length = len;

    if ((ret = m_socket->read(chunk->data, len)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_TCP_SOCKET_COPY_FAILED, "http read no chunked data failed. ret=%d", ret);
        DFree(chunk);
        return ret;
    }

    m_recv_chunks.push_back(chunk);

    m_body_len += len;

    return ret;
}

int http_reader::readBody(char *data, int len)
{
    if (len < 0) {
        return -1;
    } else if (len == 0 || m_body_len < len) {
        return SOCKET_EAGAIN;
    }

    return readDataFromBuffer(data, len);
}

int http_reader::getLength()
{
    return m_body_len;
}

bool http_reader::empty()
{
    return m_body_len == 0;
}

int http_reader::read_http_header()
{
    int ret = ERROR_SUCCESS;

    int len = m_socket->getReadBufferLength();

    if (len <= 0) {
        return SOCKET_EAGAIN;
    }

    MemoryChunk *chunk = DMemPool::instance()->getMemory(len);
    chunk->length = len;
    DAutoFree(MemoryChunk, chunk);

    if ((ret = m_socket->copy(chunk->data, len)) != ERROR_SUCCESS) {
        ret = ERROR_HTTP_COPY_HEADER;
        log_error("copy http header data filed. ret=%d", ret);
        return ret;
    }

    DString content(chunk->data, chunk->length);

    DString delim("\r\n\r\n");
    size_t index = content.find(delim);
    if (index == string::npos) {
        return SOCKET_EAGAIN;
    }
    DString header = content.substr(0, index + delim.size());

    if (m_parser->parse(header.data(), header.length()) != ERROR_SUCCESS) {
        ret = ERROR_HTTP_HEADER_PARSER;
        log_error("parse http header failed. ret=%d", ret);
        return ret;
    }

    if (!m_parser->completed()) {
        return SOCKET_EAGAIN;
    }

    MemoryChunk *buf = DMemPool::instance()->getMemory(header.size());
    DAutoFree(MemoryChunk, buf);

    if ((ret = m_socket->read(buf->data, header.length())) != ERROR_SUCCESS) {
        ret = ERROR_HTTP_READ_HEADER;
        log_error("read http header for reduce data failed. ret=%d", ret);
        return ret;
    }

    m_h_type = HttpBody;

    DString encoding = m_parser->feild("Transfer-Encoding");
    if (encoding != "chunked") {
        m_chunked = false;
    }

    return m_h_handler(m_parser);
}

int http_reader::read_http_body()
{
    if (m_chunked) {
        return read_chunk();
    }

    return read_no_chunk();
}

int http_reader::readDataFromBuffer(char *data, int len)
{
    int ret = 0;
    int length = len;
    int pos = 0;

    while (1) {
        if (length <= 0) {
            break;
        }

        MemoryChunk *chunk = m_recv_chunks.front();
        int rest = chunk->length - m_recv_pos;

        if (length >= rest) {
            memcpy(data + pos, chunk->data + m_recv_pos, rest);
            length -= rest;
            pos += rest;
            ret += rest;

            m_recv_pos = 0;
            m_body_len -= rest;
            m_recv_chunks.pop_front();
            DFree(chunk);
        } else {
            memcpy(data + pos, chunk->data + m_recv_pos, length);
            ret += length;

            m_recv_pos += length;
            m_body_len -= length;

            break;
        }
    }

    return (ret == len) ? 0 : -1;
}
