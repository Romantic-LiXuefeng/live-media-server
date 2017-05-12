#include "DSpinLock.hpp"
#include "DGlobal.hpp"
#include <sched.h>

#define cpu_pause()        __asm__("pause")

DSpinLock::DSpinLock()
    : m_mutex(0)
{

}

DSpinLock::~DSpinLock()
{
    unlock();
}

void DSpinLock::lock()
{
    duint32 i, n;

    for (;;) {
        if (__sync_bool_compare_and_swap(&m_mutex, 0, 1)) {
            return;
        }

        for (n = 1; n < 2048; n <<= 1) {
            for (i = 0; i < n; i++) {
                cpu_pause();
            }

            if (__sync_bool_compare_and_swap(&m_mutex, 0, 1)) {
                return;
            }
        }

        sched_yield();
    }
}

void DSpinLock::unlock()
{
    m_mutex = 0;
}

/*******************************************************************/

DSpinLocker::DSpinLocker(DSpinLock *mutex)
    : m_mutex(mutex)
{
    m_mutex->lock();
}

DSpinLocker::~DSpinLocker()
{
    m_mutex->unlock();
}
