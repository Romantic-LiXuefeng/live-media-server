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

    bool ret = m_server->init();
    if (!ret) {
        log_error("epoll create failed");
        ::exit(-1);
    }
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
    for (int i = 0; i < (int)m_rtmp_listeners.size(); ++i) {
        m_rtmp_listeners.at(i)->close();
    }
    m_rtmp_listeners.clear();

    m_rtmp_ports = lms_config::instance()->get_rtmp_ports();

    for (int i = 0; i < (int)m_rtmp_ports.size(); ++i) {
        lms_rtmp_listener *listener = new lms_rtmp_listener(this);

        int ret = listener->listen(DString(""), m_rtmp_ports.at(i));
        if (ret != ERROR_SUCCESS) {
            log_error("threads server start listen rtmp failed. thread_id=%d, port=%d", thread_id(), m_rtmp_ports.at(i));
            DFree(listener);
            continue;
        }

        m_server->startListener(listener);
        m_rtmp_listeners.push_back(listener);
    }
}

void lms_threads_server::reload_rtmp()
{
    std::vector<int> rtmp_ports = lms_config::instance()->get_rtmp_ports();

    if (rtmp_ports.empty()) {
        return;
    }

    if (rtmp_ports.size() != m_rtmp_ports.size()) {
        start_rtmp();
    } else {
        for (int i = 0; i < (int)rtmp_ports.size(); ++i) {
             std::vector<int>::iterator it = find(m_rtmp_ports.begin(), m_rtmp_ports.end(), rtmp_ports.at(i));
             if (it == m_rtmp_ports.end()) {
                 start_rtmp();
                 break;
             }
        }
    }
}

void lms_threads_server::start_http()
{
    for (int i = 0; i < (int)m_http_listeners.size(); ++i) {
        m_http_listeners.at(i)->close();
    }
    m_http_listeners.clear();

    m_http_ports = lms_config::instance()->get_http_ports();

    for (int i = 0; i < (int)m_http_ports.size(); ++i) {
        lms_http_listener *listener = new lms_http_listener(this);

        int ret = listener->listen(DString(""), m_http_ports.at(i));
        if (ret != ERROR_SUCCESS) {
            log_error("threads server socket listen failed. thread_id=%d, port=%d", thread_id(), m_http_ports.at(i));
            DFree(listener);
            continue;
        }

        m_server->startListener(listener);
        m_http_listeners.push_back(listener);
    }
}

void lms_threads_server::reload_http()
{
    std::vector<int> http_ports = lms_config::instance()->get_http_ports();

    if (http_ports.empty()) {
        return;
    }

    if (http_ports.size() != m_http_ports.size()) {
        start_http();
    } else {
        for (int i = 0; i < (int)http_ports.size(); ++i) {
             std::vector<int>::iterator it = find(m_http_ports.begin(), m_http_ports.end(), http_ports.at(i));
             if (it == m_http_ports.end()) {
                 start_http();
                 break;
             }
        }
    }
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
