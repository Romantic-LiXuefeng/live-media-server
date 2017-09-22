#include "DTimer.hpp"
#include "DGlobal.hpp"

#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

static duint64 s_to_ns_base = 1 * 1000 * 1000 * 1000;
static duint64 ms_to_ns_base = 1 * 1000 * 1000;

DTimer::DTimer(DEvent *event)
    : m_event(event)
    , m_fd(-1)
    , m_handler(NULL)
    , m_singleShot(false)
{

}

DTimer::~DTimer()
{

}

void DTimer::setTimerEvent(TimerEvent handler)
{
    m_handler = handler;
}

bool DTimer::start(int msec)
{
    struct itimerspec new_value;
    struct timespec now;

    duint64 sec = 0;
    duint64 nsec = 0;

    if (msec >= 1000) {
        sec = msec / 1000;
        nsec = (msec % 1000) * ms_to_ns_base;
    } else {
        sec = 0;
        nsec = msec * ms_to_ns_base;
    }

    clock_gettime(CLOCK_MONOTONIC, &now);

    new_value.it_value.tv_sec = now.tv_sec + sec;
    new_value.it_value.tv_nsec = now.tv_nsec + nsec;

    if (new_value.it_value.tv_nsec >= s_to_ns_base) {
        new_value.it_value.tv_sec += new_value.it_value.tv_nsec / s_to_ns_base;
        new_value.it_value.tv_nsec = (new_value.it_value.tv_nsec % s_to_ns_base) * s_to_ns_base;
    }

    if (m_singleShot) {
        new_value.it_interval.tv_sec = 0;
        new_value.it_interval.tv_nsec = 0;
    } else {
        new_value.it_interval.tv_sec = sec;
        new_value.it_interval.tv_nsec = nsec;
    }

    if (new_value.it_interval.tv_nsec >= s_to_ns_base) {
        new_value.it_interval.tv_sec += new_value.it_interval.tv_nsec / s_to_ns_base;
        new_value.it_interval.tv_nsec = (new_value.it_interval.tv_nsec % s_to_ns_base) * s_to_ns_base;
    }

    m_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

    timerfd_settime(m_fd, TFD_TIMER_ABSTIME, &new_value, NULL);

    if (!m_event->add(this, m_fd)) {
        return false;
    }
    return true;
}

void DTimer::stop()
{
    struct itimerspec new_value;
    new_value.it_value.tv_sec = 0;
    new_value.it_value.tv_nsec = 0;
    new_value.it_interval.tv_sec = 0;
    new_value.it_interval.tv_nsec = 0;

    timerfd_settime(m_fd, TFD_TIMER_ABSTIME, &new_value, NULL);

    m_event->del(this, m_fd);

    if (m_fd != -1) {
        ::close(m_fd);
        m_fd = -1;
    }
}

void DTimer::setSingleShot(bool singleShot)
{
    m_singleShot = singleShot;
}

int DTimer::GetDescriptor()
{
    return m_fd;
}

int DTimer::onRead()
{
    duint64 value = 0;
    ::read(m_fd, &value, sizeof(duint64));

    if (value == 0) {
        return 0;
    }

    m_handler();

    return 0;
}

int DTimer::onWrite()
{
    return 0;
}
