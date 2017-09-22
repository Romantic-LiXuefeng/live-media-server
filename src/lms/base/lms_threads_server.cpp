#include "lms_threads_server.hpp"
#include "lms_config.hpp"
#include "lms_rtmp_listener.hpp"
#include "lms_http_listener.hpp"

#include "kernel_errno.hpp"
#include "DGlobal.hpp"
#include "kernel_log.hpp"

#include <algorithm>

lms_threads_server::lms_threads_server()
{
    m_server = new DTcpServer();
}

lms_threads_server::~lms_threads_server()
{
    DFree(m_server);
}

void lms_threads_server::reload()
{
    reload_rtmp();
    reload_http();
}

void lms_threads_server::run()
{
    lms_server_manager::instance()->addServer(m_server);

    start_rtmp();
    start_http();

    m_server->start();
}

void lms_threads_server::start_rtmp()
{
    std::vector<int> ports = lms_config::instance()->get_rtmp_ports();

    for (int i = 0; i < (int)ports.size(); ++i) {
        start_rtmp(ports.at(i));
    }
}

void lms_threads_server::reload_rtmp()
{
    std::vector<int> ports = lms_config::instance()->get_rtmp_ports();

    if (ports.empty()) {
        return;
    }

    for (int i = 0; i < (int)ports.size(); ++i) {
        int port = ports.at(i);

        std::map<int, DTcpListener*>::iterator it = m_rtmps.find(port);
        if (it == m_rtmps.end()) {
            start_rtmp(port);
        }
    }

    std::map<int, DTcpListener*>::iterator it = m_rtmps.begin();
    while (it != m_rtmps.end()) {
        int port = it->first;

        std::vector<int>::iterator iter = find(ports.begin(), ports.end(), port);
        if (iter == ports.end()) {
            m_rtmps.erase(it++);
            m_server->delListener(it->second);
        } else {
            ++it;
        }
    }
}

void lms_threads_server::start_http()
{
    std::vector<int> ports = lms_config::instance()->get_http_ports();

    for (int i = 0; i < (int)ports.size(); ++i) {
        start_http(ports.at(i));
    }
}

void lms_threads_server::reload_http()
{
    std::vector<int> ports = lms_config::instance()->get_http_ports();

    if (ports.empty()) {
        return;
    }

    for (int i = 0; i < (int)ports.size(); ++i) {
        int port = ports.at(i);

        std::map<int, DTcpListener*>::iterator it = m_https.find(port);
        if (it == m_https.end()) {
            start_rtmp(port);
        }
    }

    std::map<int, DTcpListener*>::iterator it = m_https.begin();
    while (it != m_https.end()) {
        int port = it->first;

        std::vector<int>::iterator iter = find(ports.begin(), ports.end(), port);
        if (iter == ports.end()) {
            m_https.erase(it++);
            m_server->delListener(it->second);
        } else {
            ++it;
        }
    }
}

int lms_threads_server::start_rtmp(int port)
{
    int ret = ERROR_SUCCESS;

    lms_rtmp_listener *listener = new lms_rtmp_listener(m_server->getEvent(), this);

    ret = listener->listen(DString(""), port);
    if (ret != ERROR_SUCCESS) {
        log_error("threads server start listen rtmp failed. thread_id=%d, port=%d", thread_id(), port);
        DFree(listener);
        return ret;
    }

    if (!m_server->addListener(listener)) {
        log_error("add rtmp listener to epoll failed. thread_id=%d, port=%d", thread_id(), port);
        DFree(listener);
        return ret;
    }

    m_rtmps[port] = listener;

    return ret;
}

int lms_threads_server::start_http(int port)
{
    int ret = ERROR_SUCCESS;

    lms_http_listener *listener = new lms_http_listener(m_server->getEvent(), this);

    ret = listener->listen(DString(""), port);
    if (ret != ERROR_SUCCESS) {
        log_error("threads server start listen http failed. thread_id=%d, port=%d", thread_id(), port);
        DFree(listener);
        return ret;
    }

    if (!m_server->addListener(listener)) {
        log_error("add http listener to epoll failed. thread_id=%d, port=%d", thread_id(), port);
        DFree(listener);
        return ret;
    }

    m_https[port] = listener;

    return ret;

}

/*********************************************************************************/

lms_server_manager *lms_server_manager::m_instance = new lms_server_manager;

lms_server_manager::lms_server_manager()
{

}

lms_server_manager::~lms_server_manager()
{

}

lms_server_manager *lms_server_manager::instance()
{
    return m_instance;
}

void lms_server_manager::addServer(DTcpServer *server)
{
    DSpinLocker locker(&m_mutex);
    m_servers.push_back(server);
}

std::vector<DTcpServer *> lms_server_manager::getServer()
{
    return m_servers;
}
