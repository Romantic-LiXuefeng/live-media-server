#ifndef DTCPSERVER_HPP
#define DTCPSERVER_HPP

#include "DEvent.hpp"

class DTcpListener : public EventHanderBase
{
public:
    DTcpListener(DEvent *event);
    virtual ~DTcpListener();

    int listen(const DString &ip, int port);
    void close();

    void setEvent(DEvent *event);
    DEvent* getEvent();

    int GetDescriptor();

public:
    virtual int onRead();
    virtual int onWrite();

protected:
    virtual EventHanderBase* onNewConnection(int fd) = 0;

private:
    void process(int fd);

protected:
    DEvent *m_event;
    int m_fd;

    int m_port;
    DString m_ip;
};

class DTcpServer
{
public:
    DTcpServer();
    virtual ~DTcpServer();

    void start();

    bool addListener(DTcpListener *listener);
    bool delListener(DTcpListener *listener);

    DEvent* getEvent();

private:
    DEvent *m_event;
};

#endif // DTCPSERVER_HPP
