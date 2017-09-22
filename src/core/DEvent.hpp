#ifndef DEVENT_HPP
#define DEVENT_HPP

#include "DGlobal.hpp"
#include "DDateTime.hpp"

#include <map>
#include <vector>
#include <time.h>

class DTimer;

class EventHanderBase
{
public:
    EventHanderBase() {}
    virtual ~EventHanderBase() {}

public:
    virtual int onRead() = 0;
    virtual int onWrite() = 0;
};

class EventTimeOutBase
{
public:
    EventTimeOutBase() {}
    virtual ~EventTimeOutBase() {}

public:
    virtual void onReadTimeOut() = 0;
    virtual void onWriteTimeOut() = 0;
};

class DEvent
{
public:
    DEvent();
    virtual ~DEvent();

    void start();
    void close();

    int GetDescriptor();

    /**
     * @brief add
     * @param handler
     * @param fd
     * @return 成功返回true，失败返回false，如果失败，外部调用者需要调用del进行删除
     */
    bool add(EventHanderBase *handler, int fd);
    bool del(EventHanderBase *handler, int fd);

    /**
     * @brief 将handler加入到超时队列中等待处理，单位是微妙
     * @param handler
     * @param timeout 通过clock_gettime(CLOCK_MONOTONIC)得到的微妙数
     */
    void addReadTimeOut(EventTimeOutBase *handler, dint64 timeout);
    void addWriteTimeOut(EventTimeOutBase *handler, dint64 timeout);
    /**
     * @brief 将m_map和m_key_map中的handler项删除
     * @param handler
     */
    void delReadTimeOut(EventTimeOutBase *handler);
    void delWriteTimeOut(EventTimeOutBase *handler);

private:
    void onTimeOut();
    void generateMonotonicTime();
    void startTimer();

    void wait();
    void freeDelHandlers();

private:
    int m_fd;
    std::vector<EventHanderBase*> m_del_handlers;

private:
    DTimer *m_timer;

    typedef pair<EventTimeOutBase*, dint64> PAIR;
    struct CmpByValue {
        bool operator()(const PAIR& lhs, const PAIR& rhs) {
            return lhs.second < rhs.second;
        }
    };
    std::map<EventTimeOutBase*, dint64> m_read_timeout_handlers;
    std::map<EventTimeOutBase*, dint64> m_write_timeout_handlers;

private:
    struct timespec m_monotonic;
    dint64 m_usec;
};

#endif // DEVENT_HPP
