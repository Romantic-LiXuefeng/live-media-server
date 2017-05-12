#include "DMemPool.hpp"
#include "DGlobal.hpp"
#include <algorithm>

#define DEFAULT_BLOCK_SIZE   1048576   // 1024 * 1024
#define MAX_MEMORY_SIZE      1048576   // 1024 * 1024

MemoryChunk::MemoryChunk()
    : length(0)
    , data(NULL)
    , size(0)
{

}

MemoryChunk::~MemoryChunk()
{
    DMemPool::instance()->destroyMemory(this);
}

MemoryBlock::MemoryBlock()
    : pos(0)
    , count(0)
{
    data = new char[DEFAULT_BLOCK_SIZE];
}

MemoryBlock::~MemoryBlock()
{
    DFreeArray(data);
}

/**************************************************************/

DMemPool *DMemPool::m_instance = new DMemPool();

DMemPool::DMemPool()
{
    MemoryBlock *block = new MemoryBlock();
    m_cur_count = m_total_count = 0;
    m_pools[m_total_count] = block;
}

DMemPool::~DMemPool()
{

}

MemoryChunk *DMemPool::getMemory(int size)
{
    DSpinLocker locker(&m_mutex);

    MemoryChunk *chunk = new MemoryChunk();
    chunk->size = size;

    if (size > MAX_MEMORY_SIZE) {
        chunk->data = new char[size];
        return chunk;
    }

    MemoryBlock *block = m_pools[m_cur_count];

    if (block->pos + size <= DEFAULT_BLOCK_SIZE) {
        chunk->data = block->data + block->pos;
        chunk->number = m_cur_count;

        block->pos += size;
        block->count++;

        return chunk;
    }

    if (!m_idles.empty()) {
        duint64 number = m_idles.front();
        m_idles.pop_front();

        block = m_pools[number];
        block->pos += size;
        block->count++;

        chunk->data = block->data;
        chunk->number = number;

        m_cur_count = number;

        return chunk;
    }

    block = new MemoryBlock();
    block->pos += size;
    block->count++;

    m_total_count++;
    m_cur_count = m_total_count;
    m_pools[m_total_count] = block;

    chunk->data = block->data;
    chunk->number = m_cur_count;

    return chunk;
}

void DMemPool::destroyMemory(MemoryChunk *chunk)
{
    DSpinLocker locker(&m_mutex);

    if (chunk->size > MAX_MEMORY_SIZE) {
        DFree(chunk->data);
        return;
    }

    MemoryBlock *block = m_pools[chunk->number];
    block->count--;

    if (block->count == 0) {
        block->pos = 0;

        if (chunk->number != m_cur_count) {
            m_idles.push_back(chunk->number);

            if (m_idles.size() > 100) {
                std::list<duint64>::iterator it;
                for (it = m_idles.begin(); it != m_idles.end(); ++it) {
                    DFree(m_pools[*it]);
                    m_pools.erase(*it);
                }
                m_idles.clear();
            }
        }
    }
}

DMemPool *DMemPool::instance()
{
    return m_instance;
}

void DMemPool::print()
{
    DSpinLocker locker(&m_mutex);
    printf("pool_count=%d, idle_count=%d\n", m_pools.size(), m_idles.size());
}
