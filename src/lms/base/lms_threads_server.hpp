#ifndef LMS_THREADS_SERVER_HPP
#define LMS_THREADS_SERVER_HPP

#include "DThread.hpp"
#include "DTcpServer.hpp"
#include "DSpinLock.hpp"

#include <vector>
#include <map>

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
    int start_rtmp(int port);
    int start_http(int port);

private:
    DTcpServer *m_server;

    std::map<int, DTcpListener*> m_rtmps;
    std::map<int, DTcpListener*> m_https;
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
