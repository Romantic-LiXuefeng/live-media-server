#include "DSignal.hpp"
#include <sys/signalfd.h>
#include <signal.h>

DSignal::DSignal(DEvent *event)
    : m_event(event)
    , m_fd(-1)
    , m_handler(NULL)
    , m_sig(-1)
{

}

DSignal::~DSignal()
{

}

bool DSignal::open(int sig)
{
    m_sig = sig;

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, sig);

    sigprocmask(SIG_BLOCK, &mask, NULL);

    m_fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);

    if (!m_event->add(this, m_fd)) {
        return false;
    }
    return true;
}

void DSignal::close()
{
    m_event->del(this, m_fd);

    if (m_fd != -1) {
        ::close(m_fd);
        m_fd = -1;
    }
}

void DSignal::setHandler(SignalHandler handler)
{
    m_handler = handler;
}

int DSignal::GetDescriptor()
{
    return m_fd;
}

int DSignal::onRead()
{
    struct signalfd_siginfo value;
    ssize_t ret = read(m_fd, &value, sizeof(struct signalfd_siginfo));
    if (ret != sizeof(struct signalfd_siginfo)) {
        return 0;
    }

    if (value.ssi_signo == (uint32_t)m_sig) {
        m_handler();
    }

    return 0;
}

int DSignal::onWrite()
{
    return 0;
}
