#ifndef DSIGNAL_HPP
#define DSIGNAL_HPP

#include "DEvent.hpp"
#include <tr1/functional>

typedef std::tr1::function<void (void)> SignalHandler;

#define SIGNAL_CALLBACK(str) \
    std::tr1::bind(str, this)

/**
 * @brief 由于线程对信号的影响，建议信号操作在主线程中进行，并且在子线程启动之前
 *        此类不需要释放，调用close函数即可
 */
class DSignal : public EventHanderBase
{
public:
    DSignal(DEvent *event);
    ~DSignal();

    bool open(int sig);
    void close();
    void setHandler(SignalHandler handler);

public:
    virtual int GetDescriptor();
    virtual int onRead();
    virtual int onWrite();

private:
    DEvent *m_event;
    int m_fd;
    SignalHandler m_handler;
    int m_sig;
};

#endif // DSIGNAL_HPP
