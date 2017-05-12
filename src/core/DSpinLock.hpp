#ifndef DSPINLOCK_HPP
#define DSPINLOCK_HPP

#include <stdlib.h>
#include <unistd.h>

/**
 * @brief 原子锁是自旋锁，不是互斥锁
 */
class DSpinLock
{
public:
    DSpinLock();
    ~DSpinLock();

public:
    void lock();
    void unlock();

private:
    volatile int m_mutex;
};

class DSpinLocker
{
public:
    DSpinLocker(DSpinLock *mutex);
    ~DSpinLocker();

private:
    DSpinLock *m_mutex;
};

#endif // DSPINLOCK_HPP
