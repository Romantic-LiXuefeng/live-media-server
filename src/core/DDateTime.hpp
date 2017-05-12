#ifndef DDATETIME_HPP
#define DDATETIME_HPP

#include "DString.hpp"

class DDateTime
{
public:
    DDateTime();
    ~DDateTime();

    /**
     * currently only supported yyyy MM dd hh mm ss ms
     *
     * eg. 2014-03-30 12:12:34.560
     */
    DString toString(const DString &format);

    duint64 toMS();
    duint64 toUs();
    duint64 toSecond();

public:
    static DDateTime currentDate();

    /**
     * @brief monotonic 系统启动开始到当前的时间
     * @return
     */
    static struct timespec monotonic();

    static DString toString(const DString &format, struct timeval tv);

private:
    struct timeval m_tv;
};

#endif // DDATETIME_HPP
