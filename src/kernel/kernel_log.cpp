#include "kernel_log.hpp"

kernel_context::kernel_context()
    : m_basic_id(100)
{

}

kernel_context::~kernel_context()
{

}

void kernel_context::set_id(int fd)
{
    DSpinLocker locker(&m_mutex);

    m_fd_ids[fd] = m_basic_id++;
}

void kernel_context::update_id(int fd)
{
    DSpinLocker locker(&m_mutex);

    duint64 id = 0;

    std::map<int, duint64>::iterator iter = m_fd_ids.find(fd);

    if (iter != m_fd_ids.end()) {
        id = iter->second;
    }

    m_thread_ids[pthread_self()] = id;
}

void kernel_context::update(int id)
{
    m_thread_ids[pthread_self()] = id;
}

void kernel_context::delete_id(int fd)
{
    DSpinLocker locker(&m_mutex);

    std::map<int, duint64>::iterator iter = m_fd_ids.find(fd);

    if (iter != m_fd_ids.end()) {
        m_fd_ids.erase(iter);
    }

    m_thread_ids[pthread_self()] = 0;
}

duint64 kernel_context::get_id()
{
    DSpinLocker locker(&m_mutex);

    std::map<pthread_t, duint64>::iterator iter = m_thread_ids.find(pthread_self());

    if (iter != m_thread_ids.end()) {
        return iter->second;
    }

    return 0;
}

/*************************************************************/

kernel_log* kernel_log::m_instance = new kernel_log;

kernel_log::kernel_log()
{

}

kernel_log::~kernel_log()
{

}

kernel_log *kernel_log::instance()
{
    return m_instance;
}
