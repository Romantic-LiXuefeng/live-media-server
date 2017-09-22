#include "http_flv_writer.hpp"
#include "DSharedPtr.hpp"
#include "DMemPool.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"

http_flv_writer::http_flv_writer(DTcpSocket *socket)
    : m_socket(socket)
    , m_chunked(true)
{
    m_muxer = new flv_muxer();
}

http_flv_writer::~http_flv_writer()
{
    DFree(m_muxer);
}

void http_flv_writer::set_chunked(bool chunked)
{
    m_chunked = chunked;
}

int http_flv_writer::send_av_data(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if (m_chunked) {
        // 64 => chunk size
        // 2  => \r\n
        MemoryChunk *chunk = DMemPool::instance()->getMemory(64 + 2);
        DSharedPtr<MemoryChunk> begin = DSharedPtr<MemoryChunk>(chunk);

        int chunked_size = 11 + msg->payload_length + 4;
        int nb_size = snprintf(begin->data, 64, "%x", chunked_size);

        begin->length = nb_size + 2;

        char *p = begin->data + nb_size;
        *p++ = '\r';
        *p++ = '\n';

        m_socket->add(begin, begin->length);
    }

    std::list<DSharedPtr<MemoryChunk> > msgs = m_muxer->encode(msg);
    std::list<DSharedPtr<MemoryChunk> >::iterator it;
    for (it = msgs.begin(); it != msgs.end(); ++it) {
        DSharedPtr<MemoryChunk> ch = *it;
        m_socket->add(ch, ch->length);
    }

    if (m_chunked) {
        // 2 => \r\n
        MemoryChunk *chunk = DMemPool::instance()->getMemory(2);
        DSharedPtr<MemoryChunk> end = DSharedPtr<MemoryChunk>(chunk);

        end->length = 2;

        char *p = end->data;
        *p++ = '\r';
        *p++ = '\n';

        m_socket->add(end, end->length);
    }

    if ((ret = m_socket->flush()) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_HTTP_WRITE_FLV_TAG, "write http flv tag failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int http_flv_writer::send_flv_header()
{
    int ret = ERROR_SUCCESS;

    if (m_chunked) {
        // 66 = 64 + 2
        MemoryChunk *chunk = DMemPool::instance()->getMemory(66);
        DSharedPtr<MemoryChunk> begin = DSharedPtr<MemoryChunk>(chunk);

        // 13 = flv header
        int nb_size = snprintf(begin->data, 64, "%x", 13);

        begin->length = nb_size + 2;

        char *p = begin->data + nb_size;
        *p++ = '\r';
        *p++ = '\n';

        m_socket->add(begin, begin->length);
    }

    DSharedPtr<MemoryChunk> header = m_muxer->flv_header();
    m_socket->add(header, header->length);

    if (m_chunked) {
        // \r\n
        MemoryChunk *chunk = DMemPool::instance()->getMemory(2);
        DSharedPtr<MemoryChunk> end = DSharedPtr<MemoryChunk>(chunk);
        end->length = 2;

        char *p = end->data;
        *p++ = '\r';
        *p++ = '\n';

        m_socket->add(end, end->length);
    }

    if ((ret = m_socket->flush()) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_HTTP_WRITE_FLV_HEADER, "write http flv header failed. ret=%d", ret);
        return ret;
    }

    return ret;
}
