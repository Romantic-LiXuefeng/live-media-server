#ifndef LMS_THREADS_SERVER_HPP
#define LMS_THREADS_SERVER_HPP

#include "DThread.hpp"
#include "DTcpServer.hpp"
#include "DSpinLock.hpp"

#include <vector>

class lms_threads_server : public DThread
{
public:
    lms_threads_server();
    virtual ~lms_threads_server();

    void reload();

protected:
    virtual void run();

private:
    void start_rtmp();
    void reload_rtmp();
    void start_http();
    void reload_http();

private:
    DTcpServer *m_server;

    std::vector<DTcpListener*> m_rtmp_listeners;
    std::vector<int> m_rtmp_ports;

    std::vector<DTcpListener*> m_http_listeners;
    std::vector<int> m_http_ports;
};

class lms_server_manager
{
public:
    lms_server_manager();
    ~lms_server_manager();

    static lms_server_manager *instance();

    void addServer(DTcpServer *server);
    std::vector<DTcpServer*> getServer();

private:
    static lms_server_manager *m_instance;

private:
    std::vector<DTcpServer*> m_servers;
    DSpinLock m_mutex;

};

#endif // LMS_THREADS_SERVER_HPP
