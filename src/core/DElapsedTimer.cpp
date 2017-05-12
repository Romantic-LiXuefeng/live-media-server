#include "DElapsedTimer.hpp"
#include <stdio.h>

DElapsedTimer::DElapsedTimer()
    : m_started(false)
{

}

void DElapsedTimer::start()
{
    clock_gettime(CLOCK_MONOTONIC, &m_begin);
    m_started = true;
}

duint64 DElapsedTimer::restart()
{
    duint64 lastElapsed = elapsed();
    start();

    return lastElapsed;
}

duint64 DElapsedTimer::elapsed() const
{
    duint64 ret = 0;

    if (m_started) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);

        ret = now.tv_sec * 1000 * 1000 + now.tv_nsec / 1000 - (m_begin.tv_sec * 1000 * 1000 + m_begin.tv_nsec / 1000);
    }

    return ret;
}

bool DElapsedTimer::hasExpired(duint64 timeout)
{
    return elapsed() >= timeout;
}



