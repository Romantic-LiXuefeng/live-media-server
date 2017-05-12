#include "DStream.hpp"
#include <string.h>

DStream::DStream(char *bytes, int size)
{
    p = m_data = bytes;
    m_space = m_len = size;
    m_memory = NULL;
}

DStream::DStream()
{
    m_memory = NULL;
    p = m_data = NULL;
    m_len = m_space = 0;
}

DStream::~DStream()
{
    DFree(m_memory);
}

void DStream::write1Bytes(dint8 value)
{
    expand(1);
    *p++ = value;

    m_len += 1;
}

void DStream::write2Bytes(dint16 value)
{
    expand(2);
    pp = (char*)&value;
    *p++ = pp[1];
    *p++ = pp[0];

    m_len += 2;
}

void DStream::write3Bytes(dint32 value)
{
    expand(3);
    pp = (char*)&value;
    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];

    m_len += 3;
}

void DStream::write4Bytes(dint32 value)
{
    expand(4);
    pp = (char*)&value;
    *p++ = pp[3];
    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];

    m_len += 4;
}

void DStream::write8Bytes(dint64 value)
{
    expand(8);
    pp = (char*)&value;
    *p++ = pp[7];
    *p++ = pp[6];
    *p++ = pp[5];
    *p++ = pp[4];
    *p++ = pp[3];
    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];

    m_len += 8;
}

void DStream::write8Bytes(double value)
{
    expand(8);
    pp = (char*)&value;
    *p++ = pp[7];
    *p++ = pp[6];
    *p++ = pp[5];
    *p++ = pp[4];
    *p++ = pp[3];
    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];

    m_len += 8;
}

void DStream::writeString(const DString &value)
{
    expand(value.length());
    memcpy(p, value.data(), value.length());
    p += value.length();

    m_len += value.length();
}

void DStream::writeBytes(char *data, int size)
{
    expand(size);
    memcpy(p, data, size);
    p += size;

    m_len += size;
}

bool DStream::read1Bytes(dint8 &var)
{
    if (left() < 1) {
        return false;
    }

    pp = (char*)&var;
    pp[0] = *p++;

    return true;
}

bool DStream::read2Bytes(dint16 &var)
{
    if (left() < 2) {
        return false;
    }

    pp = (char*)&var;
    pp[1] = *p++;
    pp[0] = *p++;

    return true;
}

bool DStream::read3Bytes(dint32 &var)
{
    if (left() < 3) {
        return false;
    }

    pp = (char*)&var;
    pp[3] = 0;
    pp[2] = *p++;
    pp[1] = *p++;
    pp[0] = *p++;

    return true;
}

bool DStream::read4Bytes(dint32 &var)
{
    if (left() < 4) {
        return false;
    }

    pp = (char*)&var;
    pp[3] = *p++;
    pp[2] = *p++;
    pp[1] = *p++;
    pp[0] = *p++;

    return true;
}

bool DStream::read8Bytes(dint64 &var)
{
    if (left() < 8) {
        return false;
    }

    pp = (char*)&var;
    pp[7] = *p++;
    pp[6] = *p++;
    pp[5] = *p++;
    pp[4] = *p++;
    pp[3] = *p++;
    pp[2] = *p++;
    pp[1] = *p++;
    pp[0] = *p++;

    return true;
}

bool DStream::read8Bytes(double &var)
{
    if (left() < 8) {
        return false;
    }

    pp = (char*)&var;
    pp[7] = *p++;
    pp[6] = *p++;
    pp[5] = *p++;
    pp[4] = *p++;
    pp[3] = *p++;
    pp[2] = *p++;
    pp[1] = *p++;
    pp[0] = *p++;

    return true;
}

bool DStream::readString(int len, DString &var)
{
    if (left() < len) {
        return false;
    }

    var.append(p, len);
    p += len;

    return true;
}

bool DStream::readBytes(char *data, int size)
{
    if (left() < size) {
        return false;
    }

    memcpy(data, p, size);
    p += size;

    return true;
}

int DStream::left()
{
    return m_len - (p - m_data);
}

bool DStream::skip(int len)
{
    if (left() < len) {
        return false;
    }

    if (p + len < m_data) {
        return false;
    }

    p += len;

    return true;
}

void DStream::reset()
{
    p = m_data;
}

bool DStream::end()
{
    return left() == 0;
}

char *DStream::data()
{
    return m_data;
}

int DStream::size()
{
    return m_len;
}

void DStream::expand(int size)
{
    int rest = 0;
    int pos = 0;

    if (p && m_data) {
        pos = p - m_data;
        rest = m_space - (p - m_data);
    }

    if (rest < size) {
        MemoryChunk *chunk = DMemPool::instance()->getMemory(m_len + size);

        if (m_memory) {
            memcpy(chunk->data, m_memory->data, pos);
            DFree(m_memory);
        }

        m_data = chunk->data;
        m_space = chunk->size;
        p = m_data + pos;
        m_memory = chunk;
    }
}
