#include <iostream>
using namespace std;
#include "DMemPool.hpp"
#include <string.h>
#include "DGlobal.hpp"
int main()
{
    // 1
    char *p = "1234";
    MemoryChunk *chunk = DMemPool::instance()->getMemory(4);
    chunk->length = 4;
    memcpy(chunk->data, p, 4);

    DMemPool::instance()->print();  // 1   4

    // 2
    char *pp = "abcd";
    MemoryChunk *ch = DMemPool::instance()->getMemory(4);
    ch->length = 4;
    memcpy(ch->data, pp, 4);

    DMemPool::instance()->print();  // 1   0

    DFree(chunk);

    DMemPool::instance()->print();  // 1   4

    // 3
    char *ppp = "zxcv";
    MemoryChunk *ch1 = DMemPool::instance()->getMemory(4);
    ch1->length = 4;
    memcpy(ch1->data, ppp, 4);

    DMemPool::instance()->print();

    DFree(ch);

    DMemPool::instance()->print();

    string str(ch1->data, 4);
    cout << str << endl;

    DFree(ch1);

    DMemPool::instance()->print();

    return 0;
}

