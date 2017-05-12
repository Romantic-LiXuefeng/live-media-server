#include "DThread.hpp"

#include <unistd.h>
#include <sys/prctl.h>

DThread::DThread()
    : m_isRunning(false)
    , m_detach(false)
    , m_thread_t(0)
{

}

DThread::~DThread()
{
    wait();
}

int DThread::start()
{
    m_isRunning = false;

    if (m_thread_t == 0) {
        if (m_detach) {
            // 创建detached线程
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

            return pthread_create(&m_thread_t, &attr, onThreadCb, this);
        }

        // pthread_craete第二个参数为NULL，则创建默认属性的线程，此时为PTHREAD_CREATE_JOINABLE
        return pthread_create(&m_thread_t, NULL, onThreadCb, this);
    }

    return -1;
}

int DThread::wait()
{
    // 如果是jionale的线程，那么必须使用pthread_join()等待线程结束，否则线程所占用的资源不会得到释放，会造成资源泄露
    if (m_thread_t != 0 && !m_detach) {
        return pthread_join(m_thread_t, NULL);
    }

    return 0;
}

void DThread::cancel()
{
    if (m_thread_t != 0) {
        pthread_cancel(m_thread_t);
    }
}

pthread_t DThread::thread_id() const
{
    return m_thread_t;
}

bool DThread::isRunning()
{
    return m_isRunning;
}

void DThread::set_detach(bool detach)
{
    m_detach = detach;

    if (m_detach && m_thread_t != 0) {
        pthread_detach(m_thread_t);
    }
}

void DThread::setThreadName(const DString &name)
{
    m_thread_name = name;
}

void *DThread::onThreadCb(void *arg)
{
    DThread *ptr = (DThread *)arg;

    if (!ptr) {
        return NULL;
    }

    if (!ptr->m_thread_name.empty()) {
        prctl(PR_SET_NAME, ptr->m_thread_name.c_str());
    }

    ptr->m_isRunning = true;
    ptr->run();
    ptr->m_isRunning = false;

    pthread_exit(0);
}
