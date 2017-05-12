#ifndef DTCPSERVER_HPP
#define DTCPSERVER_HPP

#include "DEvent.hpp"

class DTcpListener : public EventHanderBase
{
public:
    DTcpListener();
    virtual ~DTcpListener();

    int listen(const DString &ip, int port);
    void close();

    void addToEvent(DEvent *event);
    DEvent* getEvent();

public:
    virtual int GetDescriptor();
    virtual int onRead();
    virtual int onWrite();

protected:
    virtual EventHanderBase* onNewConnection(int fd) = 0;

private:
    void setNonblocking(int fd);
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

    bool init();
    bool start();

    void startListener(DTcpListener *listener);

    DEvent* getEvent();

private:
    DEvent *m_event;
};

#endif // DTCPSERVER_HPP
