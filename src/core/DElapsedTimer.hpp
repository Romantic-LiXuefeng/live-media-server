#ifndef DELAPSEDTIMER_HPP
#define DELAPSEDTIMER_HPP

#include <time.h>
#include "DGlobal.hpp"

class DElapsedTimer
{
public:
    DElapsedTimer();

    void start();

    /**
     * @brief 重新启动计时器，并返回已经走过的时间，单位微妙
     */
    duint64 restart();

    /**
     * @brief 计时器走过的时间，单位微妙
     */
    duint64 elapsed() const;

    /**
     * @brief 计时器是不是已经到达参数中指定的时间，单位微妙
     */
    bool hasExpired(duint64 timeout);

private:
    struct timespec m_begin;

    bool m_started;
};

#endif // DELAPSEDTIMER_HPP
