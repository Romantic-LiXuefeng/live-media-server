#ifndef DMEMPOOL_HPP
#define DMEMPOOL_HPP

#include "DSpinLock.hpp"
#include <map>

class MemoryBlock;

class MemoryChunk
{
public:
    MemoryChunk();
    ~MemoryChunk();

public:
    // 实际使用的大小，存在的意义主要是为了socket读时分配的大小和实际读出的大小不一致
    int length;
    // 内存块的地址
    char *data;
    // 内存块的大小
    int size;

    MemoryBlock *block;
};

class MemoryBlock
{
public:
    MemoryBlock(int size);
    ~MemoryBlock();

public:
    //内存块的起始地址
    char *data;
    //当前使用的内存位置
    int pos;
    // 剩余的数量，一个block会分配出多个chunk，chunk回收之后只有count等于0时block才会进入空闲
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

    void setEnable(bool value);

    void print();

    void reduce();

private:
    std::multimap<int, MemoryBlock*> m_pools;
    std::map<MemoryBlock*, int> m_keys;

    DSpinLock m_mutex;

private:
    static DMemPool *m_instance;

    bool m_enable;
    int m_block_size;
};

#endif // DMEMPOOL_HPP
