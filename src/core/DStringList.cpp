#include "DStringList.hpp"

#include <iostream>

DStringList::DStringList()
{
}

DStringList::~DStringList()
{
}

bool DStringList::isEmpty() const
{
    return empty();
}

int DStringList::length()
{
    return size();
}

DString &DStringList::operator [](int i)
{
    list<DString>::iterator iter = begin();
    for (int c = 0; c < i; ++c) {
        ++iter;
    }
    return *iter;
}

const DString &DStringList::operator [](int i) const
{
    DStringListConstItor iter = begin();
    for (int c = 0; c < i; ++c) {
        ++iter;
    }
    return *iter;
}

DStringList &DStringList::operator <<(const DString &str)
{
    this->push_back(str);
    return *this;
}

DStringList &DStringList::operator <<(const DStringList &other)
{
    for (unsigned int i = 0; i < other.size(); ++i) {
        this->push_back(other.at(i));
    }

    return *this;
}

DStringList &DStringList::operator=(const DStringList &other)
{
    this->clear();
    return *this << other;
}

DString &DStringList::at(int pos)
{
    DAssert((unsigned int)pos < size());
    return (*this)[pos];
}

const DString &DStringList::at(int pos) const
{
    DAssert((unsigned int)pos < size());
    return (*this)[pos];
}

DString DStringList::join(const DString &sep)
{
    if (size() == 1) {
        return front();
    }

    DString ret;
    for (list<DString>::iterator iter = begin(); iter != end(); ++iter) {
        ret.append(*iter);
        ret.append(sep);
    }

    return ret;
}


