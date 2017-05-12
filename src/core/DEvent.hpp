#ifndef DEVENT_HPP
#define DEVENT_HPP

#include "DGlobal.hpp"
#include "DDateTime.hpp"

#include <map>
#include <vector>
#include <time.h>
#include <tr1/functional>

typedef std::tr1::function<void (void)> EventTimerHandler;
#define EVENT_TIMER_CALLBACK(str) std::tr1::bind(str, this)

class DTimer;

class EventHanderBase
{
public:
    EventHanderBase() {}
    virtual ~EventHanderBase() {}

public:
    virtual int GetDescriptor() = 0;
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

    bool init();
    void close();
    bool add(EventHanderBase *handler);
    bool del(EventHanderBase *handler);
    void remove(EventHanderBase *handler);
    bool wait();
    bool event_loop();

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
    void generateTime();

    void startTimer();

private:
    int m_fd;
    std::vector<EventHanderBase*> m_handlers;

private:
    DTimer *m_timer;

    typedef pair<EventTimeOutBase*, dint64> PAIR;
    struct CmpByValue {
        bool operator()(const PAIR& lhs, const PAIR& rhs) {
            return lhs.second < rhs.second;
        }
    };
    std::map<EventTimeOutBase*, dint64> m_read_map;
    std::map<EventTimeOutBase*, dint64> m_write_map;

private:
    struct timespec m_monotonic;
    dint64 m_usec;
};

#endif // DEVENT_HPP
