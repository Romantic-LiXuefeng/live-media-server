#include "DString.hpp"

#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>

#include "DStringList.hpp"

DString::DString()
{
}

DString::DString(const char *str)
    : string(str)
{

}

DString::DString(const char *str, int n)
    : string(str, n)
{

}

DString::DString(const string &str)
    : string(str)
{

}

DString::DString(size_t n, char c)
    : string(n, c)
{

}

DString::~DString()
{
}

string DString::toStdString()
{
    return string(c_str());
}

void DString::chop(int n)
{
    if (n >= size()) {
        clear();

        return;
    }

    erase(size() - n, n);
}

void DString::truncate(int position)
{
    DStringIterator first = begin() + position;
    erase(first, end());
}

void DString::truncate(const DString &key)
{
    size_type pos = find(key);

    if (pos == npos) return;

    DStringIterator first = begin() + pos;
    erase(first, end());
}

// '\t', '\n', '\v', '\f', '\r', and ' '
DString DString::trimmed() const
{
    int beginIndex = 0;
    int endIndex = 0;

    // find beginIndex
    for (int i = 0; i < size(); ++i) {
        char c = this->at(i);
        if (c == '\t'
                || c == '\n'
                || c == '\v'
                || c == '\f'
                || c == '\r'
                || c == ' ') {
            ++beginIndex;
        } else {
            break;
        }
    }

    // find endIndex
    for (int i = size() - 1; i >= 0; --i) {
        char c = this->at(i);
        if (c == '\t'
                || c == '\n'
                || c == '\v'
                || c == '\f'
                || c == '\r'
                || c == ' ') {
            ++endIndex;
        } else {
            break;
        }
    }

    return substr(beginIndex, size() - beginIndex - endIndex);
}

DString &DString::sprintf(const char *cformat, ...)
{
    va_list vp;
    va_start(vp, cformat);

    char buffer[1024];
    vsprintf(buffer, cformat, vp);
    va_end(vp);
    this->append(buffer);

    return *this;
}

bool DString::contains(const DString &str) const
{
    return find(str) != npos;
}

bool DString::startWith(const DString &str) const
{
    if (size() < str.size()) return false;

    DString ss = substr(0, str.size());

    return ss == str;
}

bool DString::endWith(const DString &str) const
{
    if (size() < str.size()) return false;

    DString ss = substr(size() - str.size(), str.size());

    return ss == str;
}

DString &DString::replace(const DString &before, const DString &after, bool replaceAll)
{
    size_t  pos = npos;
    while ((pos = find(before.c_str())) != npos) {
        string::replace(pos, before.size(), after);

        if (!replaceAll) {
            break;
        }
    }

    return *this;
}

int DString::toInt(bool *ok, int base)
{
    char *endptr;
    int ret = strtol(c_str(), &endptr, base);

    if ((errno == ERANGE && (ret == LONG_MAX || ret == LONG_MIN))
            || (errno != 0 && ret == 0)) {
        if (ok) {
            *ok = false;
        }
        return ret;
    }

    if (endptr == c_str()) {
        if (ok) {
            *ok = false;
        }
        return ret;
    }

    if (ok) {
        *ok = true;
    }

    return ret;
}

short DString::toShort(bool *ok, int base)
{
    return (short)toInt(ok, base);
}

DString DString::toHex()
{
    DString ret;
    DStringIterator iter = begin();
    while (iter != end()) {
        char buf[2] = {'\0', '\0'};
        ::sprintf(buf, "%x", *iter);
        ret.append(buf);
        ++iter;
    }

    return ret;
}

dint64 DString::toInt64(bool *ok, int base)
{
    char *endptr;
    dint64 ret = strtoll(c_str(), &endptr, base);
    if ((errno == ERANGE && (ret == LONG_MAX || ret == LONG_MIN))
            || (errno != 0 && ret == 0)) {
        if (ok) {
            *ok = false;
        }
        return ret;
    }

    if (endptr == c_str()) {
        if (ok) {
            *ok = false;
        }
        return ret;
    }

    if (ok) {
        *ok = true;
    }

    return ret;
}

duint64 DString::toUint64(bool *ok, int base)
{
    char *endptr;
    duint64 ret = strtoul(c_str(), &endptr, base);
    if ((errno == ERANGE && (ret == LONG_MAX || ret == LONG_MIN))
            || (errno != 0 && ret == 0)) {
        if (ok) {
            *ok = false;
        }
        return ret;
    }

    if (endptr == c_str()) {
        if (ok) {
            *ok = false;
        }
        return ret;
    }

    if (ok) {
        *ok = true;
    }

    return ret;
}

double DString::toDouble(bool *ok)
{
    char *endptr;
    double ret = strtod(c_str(), &endptr);
    if ((errno == ERANGE && (ret == LONG_MAX || ret == LONG_MIN))
            || (errno != 0 && ret == 0)) {
        if (ok) {
            *ok = false;
        }
        return ret;
    }

    if (endptr == c_str()) {
        if (ok) {
            *ok = false;
        }
        return ret;
    }

    if (ok) {
        *ok = true;
    }

    return ret;
}

DStringList DString::split(const DString &sep)
{
    DString temp(*this);
    DStringList ret;
    if (sep.isEmpty()) {
        return ret;
    }

    while (temp.contains(sep)) {
        size_type index = temp.find(sep);

        DString ss = temp.substr(0, index);
        if (!ss.isEmpty()) {
            ret << ss;
        }
        temp = temp.substr(index + sep.size(), temp.size() - 1);
    }
    if (!temp.isEmpty()) {
        ret << temp;
    }

    return ret;
}

DString &DString::prepend(const DString &str)
{
    DString temp = *this;
    *this = str + temp;
    return *this;
}

DString &DString::prepend(const char *str, int size)
{
    DString temp(str, size);
    return prepend(temp);
}

DString & DString::operator <<(dint32 value)
{
    append(number(value));
    return *this;
}

DString &DString::operator <<(duint32 value)
{
    append(number(value));
    return *this;
}

DString &DString::operator <<(dint64 value)
{
    append(number(value));
    return *this;
}

DString & DString::operator <<(duint64 value)
{
    append(number(value));
    return *this;
}

DString &DString::operator <<(double value)
{
    append(number(value));
    return *this;
}

DString & DString::operator <<(const DString &str)
{
    append(str);
    return *this;
}

DString DString::number(dint32 n)
{
    char buf[21] = {0};
    snprintf(buf, sizeof(buf), "%d", n);

    return buf;
}

DString DString::number(duint32 n)
{
    char buf[21] = {0};
    snprintf(buf, sizeof(buf), "%u", n);

    return buf;
}

DString DString::number(duint64 n)
{
    char buf[21] = {0};
    snprintf(buf, sizeof(buf), "%"PRIu64"", n);

    return buf;
}

DString DString::number(dint64 n)
{
    char buf[21] = {0};
    snprintf(buf, sizeof(buf), "%"PRId64"", n);

    return buf;
}

DString DString::number(double n)
{
    char buf[21] = {0};
    if(n - floor(n) == 0){
        snprintf(buf, sizeof(buf), "%.0f", n);
    }else{
        snprintf(buf, sizeof(buf), "%f", n);
    }

    return buf;
}


