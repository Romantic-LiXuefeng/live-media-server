#ifndef KERNEL_LOG_HPP
#define KERNEL_LOG_HPP

#include "DLog.hpp"
#include "DSpinLock.hpp"
#include <pthread.h>
#include <map>

class kernel_context
{
public:
    kernel_context();
    ~kernel_context();

public:
    void set_id(int fd);
    void update_id(int fd);
    void update(int id);
    void delete_id(int fd);
    duint64 get_id();

private:
    std::map<pthread_t, duint64> m_thread_ids;
    std::map<int, duint64> m_fd_ids;
    duint64 m_basic_id;
    DSpinLock m_mutex;
};

extern kernel_context* global_context;

class kernel_log : public DLog
{
public:
    kernel_log();
    virtual ~kernel_log();

    static kernel_log *instance();

private:
    static kernel_log *m_instance;
};

#define log_verbose(msg, ...) \
    kernel_log::instance()->verbose(__FILE__, __LINE__, __FUNCTION__, global_context->get_id(), msg, ##__VA_ARGS__)

#define log_info(msg, ...) \
    kernel_log::instance()->info(__FILE__, __LINE__, __FUNCTION__, global_context->get_id(), msg, ##__VA_ARGS__)

#define log_trace(msg, ...) \
    kernel_log::instance()->trace(__FILE__, __LINE__, __FUNCTION__, global_context->get_id(), msg, ##__VA_ARGS__)

#define log_warn(msg, ...) \
    kernel_log::instance()->warn(__FILE__, __LINE__, __FUNCTION__, global_context->get_id(), msg, ##__VA_ARGS__)

#define log_error(msg, ...) \
    kernel_log::instance()->error(__FILE__, __LINE__, __FUNCTION__, global_context->get_id(), msg, ##__VA_ARGS__)

#endif // KERNEL_LOG_HPP
