#ifndef DMUTEX_HPP
#define DMUTEX_HPP

#include <pthread.h>

class DMutex
{
public:
    DMutex();
    ~DMutex();

public:
    bool lock();
    void unlock();

    bool wait();
    void signal();

private:
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};

class DMutexLocker
{
public:
    DMutexLocker(DMutex *mutex);
    ~DMutexLocker();

private:
    DMutex *m_mutex;
};

#endif // DMUTEX_HPP
