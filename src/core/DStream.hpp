#ifndef DSTREAM_HPP
#define DSTREAM_HPP

#include "DString.hpp"
#include "DMemPool.hpp"

/**
 * @brief 读和写操作需要独立使用，因为读用的是外部的内存，写是由stream自己维护的内存
 */
class DStream
{
public:
    DStream(char *bytes, int size);
    DStream();
    ~DStream();

public:
    void write1Bytes(dint8 value);
    void write2Bytes(dint16 value);
    void write3Bytes(dint32 value);
    void write4Bytes(dint32 value);
    void write8Bytes(dint64 value);
    void write8Bytes(double value);
    void writeString(const DString &value);
    void writeBytes(char *data, int size);

    bool read1Bytes(dint8 &var);
    bool read2Bytes(dint16 &var);
    bool read3Bytes(dint32 &var);
    bool read4Bytes(dint32 &var);
    bool read8Bytes(dint64 &var);
    bool read8Bytes(double &var);
    bool readString(int len, DString &var);
    bool readBytes(char *data, int size);

    int left();
    bool skip(int len);
    void reset();
    bool end();

    char *data();
    int size();

private:
    void expand(int size);

private:
    // 实际数据的指针
    char *m_data;
    // 内存移动的指针
    char *p;
    // 临时指针
    char *pp;
    // 实际数据的大小
    int m_len;

    MemoryChunk *m_memory;
    // 实际分配的内存空间的大小
    int m_space;
};

#endif // DSTREAM_HPP
