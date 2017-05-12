#include <pthread.h>
#include "DSpinLock.hpp"
#include "DElapsedTimer.hpp"
#include "DMutex.hpp"

DSpinLock mutex;
int count = 0;

void *onThreadCb(void *arg)
{
//    while (1) {
    for (int i = 0; i < 1000000; ++i){
        mutex.lock();
        count++;
        mutex.unlock();
    }
}

int main(int argc, char *argv[])
{
    pthread_t tids[10];

    DElapsedTimer timer;
    timer.start();

    for (int i = 0; i < 10; ++i) {
        pthread_create(&tids[i], NULL, onThreadCb, NULL);
    }

    for(int i = 0; i < 10; i++)
    {
        pthread_join(tids[i], NULL);
    }

    printf("%lld\n", timer.elapsed());

    return 0;
}
