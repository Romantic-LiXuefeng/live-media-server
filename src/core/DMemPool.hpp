#ifndef DMEMPOOL_HPP
#define DMEMPOOL_HPP

#include "DSpinLock.hpp"
#include "DGlobal.hpp"
#include "DString.hpp"

#include <map>
#include <list>

class MemoryChunk
{
public:
    MemoryChunk();
    ~MemoryChunk();

public:
    // 实际使用的大小
    int length;
    // 内存块的地址
    char *data;
    // 内存块的大小
    int size;
    // 所属内存块的号码
    duint64 number;
};

class MemoryBlock
{
public:
    MemoryBlock();
    ~MemoryBlock();

public:
    //内存块的起始地址
    char *data;
    //当前使用的内存位置
    int pos;
    // 剩余的数量
    int count;
};

class DMemPool
{
public:
    DMemPool();
    ~DMemPool();

    MemoryChunk *getMemory(int size);
    void destroyMemory(MemoryChunk *chunk);

    static DMemPool *instance();

    void print();

private:
    std::map<duint64, MemoryBlock*> m_pools;
    std::list<duint64> m_idles;

    duint64 m_total_count;
    duint64 m_cur_count;

    DSpinLock m_mutex;

private:
    static DMemPool *m_instance;

};

#endif // DMEMPOOL_HPP
