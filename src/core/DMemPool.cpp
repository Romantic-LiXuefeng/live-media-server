#include "DMemPool.hpp"
#include "DGlobal.hpp"

#include <algorithm>

#define DEFAULT_BLOCK_SIZE   1048576   // 1024 * 1024

MemoryChunk::MemoryChunk()
    : length(0)
    , data(NULL)
    , size(0)
    , block(NULL)
{

}

MemoryChunk::~MemoryChunk()
{
    DMemPool::instance()->destroyMemory(this);
}

MemoryBlock::MemoryBlock(int size)
    : pos(0)
    , count(0)
{
    data = new char[size];
}

MemoryBlock::~MemoryBlock()
{
    DFreeArray(data);
}

/**************************************************************/

DMemPool *DMemPool::m_instance = new DMemPool();

DMemPool::DMemPool()
    : m_enable(true)
    , m_block_size(DEFAULT_BLOCK_SIZE)
{

}

DMemPool::~DMemPool()
{

}

MemoryChunk *DMemPool::getMemory(int size)
{
    DSpinLocker locker(&m_mutex);

    MemoryChunk *chunk = new MemoryChunk();
    chunk->size = size;

    if ((size > m_block_size) || !m_enable) {
        chunk->data = new char[size];
        return chunk;
    }

    std::multimap<int, MemoryBlock*>::iterator it;
    it = m_pools.lower_bound(size);

    if (it != m_pools.end()) {
        MemoryBlock *block = (*it).second;

        chunk->data = block->data + block->pos;
        chunk->block = block;

        block->pos += size;
        block->count++;

        int rest = m_block_size - block->pos;

        m_pools.erase(it);
        m_pools.insert(std::make_pair(rest, block));

        m_keys[block] = rest;

        return chunk;
    }

    MemoryBlock *block = new MemoryBlock(m_block_size);
    chunk->data = block->data;
    chunk->block = block;

    block->pos += size;
    block->count++;

    int rest = m_block_size - block->pos;
    m_pools.insert(std::make_pair(rest, block));

    m_keys[block] = rest;

    return chunk;
}

void DMemPool::destroyMemory(MemoryChunk *chunk)
{
    DSpinLocker locker(&m_mutex);

    if ((chunk->size > m_block_size) || (chunk->block == NULL)) {
        DFree(chunk->data);
        return;
    }

    int key = m_keys[chunk->block];

    std::multimap<int, MemoryBlock*>::iterator itlow, itup;

    itlow = m_pools.lower_bound(key);
    itup = m_pools.upper_bound(key);

    while (itlow != itup) {
        MemoryBlock *block = itlow->second;
        if (block == chunk->block) {
            block->count--;

            if (block->count == 0) {
                block->pos = 0;

                m_keys[block] = m_block_size;

                m_pools.erase(itlow);
                m_pools.insert(std::make_pair(m_block_size, block));
            }

            break;
        }

        itlow++;
    }

    reduce();
}

DMemPool *DMemPool::instance()
{
    return m_instance;
}

void DMemPool::setEnable(bool value)
{
    DSpinLocker locker(&m_mutex);

    m_enable = value;

    if (!m_enable) {
        m_block_size = 0;
    }
}

void DMemPool::print()
{
    DSpinLocker locker(&m_mutex);

    printf("DMemPool: total=%d, idles=%d\n", m_pools.size(), m_pools.count(m_block_size));
}

void DMemPool::reduce()
{
    // 测试
    if (m_pools.count(m_block_size) < 100) {
        return;
    }

    std::multimap<int, MemoryBlock*>::iterator itlow, itup;

    itlow = m_pools.lower_bound(m_block_size);
    itup = m_pools.upper_bound(m_block_size);

    while (itlow != itup) {
        if (itlow->first == m_block_size) {
            MemoryBlock *block = itlow->second;
            m_pools.erase(itlow);
            m_keys.erase(block);

            DFree(block);

            break;
        }

        itlow++;
    }
}
