#include "DHostInfo.hpp"

DHostInfo::DHostInfo(DEvent *event)
    : m_event(event)
    , m_fd(-1)
    , m_finished_handler(NULL)
    , m_error_handler(NULL)
{

}

DHostInfo::~DHostInfo()
{
    ares_destroy(m_channel);
}

void DHostInfo::lookupHost(const DString &host)
{
    ares_init(&m_channel);
    ares_gethostbyname(m_channel, host.c_str(), AF_INET, callback, this);
    ares_getsock(m_channel, &m_fd, 1);

    m_event->add(this);
}

void DHostInfo::close()
{
    m_event->del(this);
}

void DHostInfo::setFinishedHandler(HostHandler handler)
{
    m_finished_handler = handler;
}

void DHostInfo::setErrorHandler(HostHandler handler)
{
    m_error_handler = handler;
}

int DHostInfo::GetDescriptor()
{
    return m_fd;
}

int DHostInfo::onRead()
{
    ares_process_fd(m_channel, m_fd, ARES_SOCKET_BAD);
    return 0;
}

int DHostInfo::onWrite()
{
    ares_process_fd(m_channel, ARES_SOCKET_BAD, m_fd);
    return 0;
}

void DHostInfo::callback(void *arg, int status, int timeouts, struct hostent *host)
{
    DHostInfo *obj = (DHostInfo*)arg;
    DStringList ips;

    if (!host || status != ARES_SUCCESS) {
        if (obj->m_error_handler) {
            obj->m_error_handler(ips);
        }
        obj->close();
        return;
    }

    if (host->h_addrtype == AF_INET) {
        int idx = 0;
        while (1) {
            char *addr_list_one = host->h_addr_list[idx];

            if (!addr_list_one) break;

            char dst[INET_ADDRSTRLEN];

            const char *ret = ares_inet_ntop(AF_INET, (const void*)addr_list_one, dst, INET_ADDRSTRLEN);
            if (ret != NULL) {
                ips.push_back(ret);
            }

            ++idx;
        }
    }

    if (obj->m_finished_handler) {
        obj->m_finished_handler(ips);
    }

    obj->close();
}

