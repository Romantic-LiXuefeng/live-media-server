#ifndef DHOSTINFO_HPP
#define DHOSTINFO_HPP

#include "DEvent.hpp"
#include "DStringList.hpp"
#include <ares.h>
#include <netdb.h>
#include <tr1/functional>

typedef std::tr1::function<void (const DStringList &ips)> HostHandler;

#define HOST_CALLBACK(str) \
    std::tr1::bind(str, this, std::tr1::placeholders::_1)

/**
 * @brief 此类不需要释放
 */
class DHostInfo : public EventHanderBase
{
public:
    DHostInfo(DEvent *event);
    ~DHostInfo();

    void lookupHost(const DString &host);
    void close();

    void setFinishedHandler(HostHandler handler);
    void setErrorHandler(HostHandler handler);

public:
    virtual int GetDescriptor();
    virtual int onRead();
    virtual int onWrite();

    static void callback(void *arg, int status, int timeouts, struct hostent *host);

private:
    DEvent *m_event;
    int m_fd;
    ares_channel m_channel;

    HostHandler m_finished_handler;
    HostHandler m_error_handler;
};

#endif // DHOSTINFO_HPP
