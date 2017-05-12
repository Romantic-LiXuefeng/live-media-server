#include "DDateTime.hpp"

#include <sys/time.h>
#include <time.h>
#include <string.h>

DDateTime::DDateTime()
{

}

DDateTime::~DDateTime()
{
}

//yyyy MM dd hh mm ss ms
DString DDateTime::toString(const DString &format)
{
    struct tm now = {0};
    localtime_r(&m_tv.tv_sec, &now);

    DString yyyy = DString().sprintf("%d",   1900 + now.tm_year);
    DString MM   = DString().sprintf("%02d", 1 + now.tm_mon);
    DString dd   = DString().sprintf("%02d", now.tm_mday);
    DString hh   = DString().sprintf("%02d", now.tm_hour);
    DString mm   = DString().sprintf("%02d", now.tm_min);
    DString ss   = DString().sprintf("%02d", now.tm_sec);
    DString ms   = DString().sprintf("%03d", (int)(m_tv.tv_usec/1000));

    DString ret = format;
    ret.replace("yyyy", yyyy);
    ret.replace("MM", MM);
    ret.replace("dd", dd);
    ret.replace("hh", hh);
    ret.replace("mm", mm);
    ret.replace("ss", ss);
    ret.replace("ms", ms);

    return ret;
}

DString DDateTime::toString(const DString &format, struct timeval tv)
{
    struct tm now = {0};
    localtime_r(&tv.tv_sec, &now);

    DString yyyy = DString().sprintf("%d",   1900 + now.tm_year);
    DString MM   = DString().sprintf("%02d", 1 + now.tm_mon);
    DString dd   = DString().sprintf("%02d", now.tm_mday);
    DString hh   = DString().sprintf("%02d", now.tm_hour);
    DString mm   = DString().sprintf("%02d", now.tm_min);
    DString ss   = DString().sprintf("%02d", now.tm_sec);
    DString ms   = DString().sprintf("%03d", (int)(tv.tv_usec/1000));

    DString ret = format;
    ret.replace("yyyy", yyyy);
    ret.replace("MM", MM);
    ret.replace("dd", dd);
    ret.replace("hh", hh);
    ret.replace("mm", mm);
    ret.replace("ss", ss);
    ret.replace("ms", ms);

    return ret;
}

duint64 DDateTime::toMS()
{
    long long time_us = ((long long)m_tv.tv_sec) * 1000 * 1000 +  m_tv.tv_usec;
    return time_us / 1000;
}

duint64 DDateTime::toUs()
{
    long long time_us = ((long long)m_tv.tv_sec) * 1000 * 1000 +  m_tv.tv_usec;
    return time_us;
}

duint64 DDateTime::toSecond()
{
    return (duint64)m_tv.tv_sec;
}

struct timespec DDateTime::monotonic()
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now;
}

DDateTime DDateTime::currentDate()
{
    DDateTime dt;
    ::gettimeofday(&dt.m_tv, NULL);

    return dt;
}



