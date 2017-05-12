#ifndef DTHREAD_HPP
#define DTHREAD_HPP

#include <pthread.h>
#include "DString.hpp"

/**
 * @brief 该线程stop之后不支持二次start
 */
class DThread
{
public:
    DThread();
    virtual ~DThread();

    int start();
    int wait();
    void cancel();

    pthread_t thread_id() const;
    bool isRunning();

    /**
     * @brief 设置线程分离
     */
    void set_detach(bool detach);

    /**
     * @brief 设置线程名字，start()之前调用
     */
    void setThreadName(const DString &name);

protected:
    virtual void run() = 0;

private:
    static void *onThreadCb(void *arg);

private:
    /**
     * @brief 线程名字
     */
    DString m_thread_name;
    /**
     * @brief 判断线程是不是在运行
     */
    bool m_isRunning;
    /**
     * @brief 判断线程是否是detached
     */
    bool m_detach;

    pthread_t m_thread_t;

};

#endif // DTHREAD_HPP
