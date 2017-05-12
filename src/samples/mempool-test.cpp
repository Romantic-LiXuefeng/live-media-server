#include <iostream>
using namespace std;
#include <map>
#include <list>
#include <unistd.h>

struct memchunk
{
    // 内存块的大小
    int length;
    // 内存块的地址
    char *data;
    // 实际使用的大小
    int size;
};

class mempool
{
public:
    mempool(){}
    ~mempool(){}

    memchunk *getMemory(int size)
    {
        char *data = NULL;

        int num = getMemoryNumber(size);

        map<int, list<char*> >::iterator it = m_pools.find(num);
        if ((it != m_pools.end()) && !it->second.empty()) {
            data = it->second.back();
            it->second.pop_back();
        } else {
            data = new char[num * 1024];
        }

        memchunk *chunk = new memchunk();
        chunk->data = data;
        chunk->length = num * 1024;
        chunk->size = 0;

        return chunk;
    }

    void destroyMemory(memchunk *chunk)
    {
        int num = getMemoryNumber(chunk->length);

        map<int, list<char*> >::iterator it = m_pools.find(num);
        if (it != m_pools.end()) {
            it->second.push_back(chunk->data);
        } else {
            list<char*> block;
            block.push_back(chunk->data);
            m_pools[num] = block;
        }
    }

    int getMemoryNumber(int size)
    {
        static int block[] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192 };

        int num = size / 1024 + (size % 1024 == 0 ? 0 : 1);
        for (int i = 0; i < 14; ++i) {
            if (num <= block[i]) {
                num = block[i];
                break;
            }
        }

        return num;
    }

private:
    map<int, list<char*> > m_pools;
};

int main(int argc, char *argv[])
{
    int p[] = { 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576, 2097152 };
    int pp[] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192 };

    int size = 1028;
    int n = size / 1024 + (size % 1024 == 0 ? 0 : 1);

    cout <<"1 "<< n << endl;
    for (int i = 0; i < 14; ++i) {
        if (n <= pp[i]) {
            cout<<"2 " << pp[i] << " " << pp[i] * 1024 << endl;
            break;
        }
    }

    cout <<"3 "<< 5120 % 1024 << " " << n << endl;


//    cout <<"&&&&&&&&&&&"<<endl;
//    map<int,list<int> > mm;

//    list<int> l1;
//    l1.push_back(1);
//    l1.push_back(2);
//    l1.push_back(3);

//    list<int> l2;
//    l2.push_back(10);
//    l2.push_back(20);
//    l2.push_back(30);

//    mm[1] = l1;
//    mm[2] = l2;

//    map<int,list<int> >::iterator it = mm.find(2);
//    it->second.push_back(100);

//    for (it = mm.begin(); it != mm.end(); ++it) {
//        for (std::list<int>::iterator it1=it->second.begin(); it1 != it->second.end(); ++it1)
//           std::cout << *it1 << endl;

//        if (!it->second.empty()) {
//            cout << "^^^^^^^^" << it->second.back()<<endl;
//            it->second.pop_back();
//        }
//    }
//    cout<<"******" <<endl;
//    for (it = mm.begin(); it != mm.end(); ++it) {
//        for (std::list<int>::iterator it1=it->second.begin(); it1 != it->second.end(); ++it1)
//           std::cout << *it1 << endl;
//    }

    return 0;
}
