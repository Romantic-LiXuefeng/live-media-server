#ifndef DTIMER_HPP
#define DTIMER_HPP

#include "DEvent.hpp"
#include <tr1/functional>

typedef std::tr1::function<void (void)> TimerEvent;

#define TIMER_CALLBACK(str) \
    std::tr1::bind(str, this)

/**
 * @brief 此类不需要释放，调用stop函数即可
 */
class DTimer : public EventHanderBase
{
public:
    DTimer(DEvent *event);
    ~DTimer();

    void setTimerEvent(TimerEvent handler);

    void start(int msec);
    void stop();
    void setSingleShot(bool singleShot);

public:
    virtual int GetDescriptor();
    virtual int onRead();
    virtual int onWrite();

private:
    DEvent *m_event;
    int m_fd;

    TimerEvent m_handler;

    bool m_singleShot;

};

#endif // DTIMER_HPP
