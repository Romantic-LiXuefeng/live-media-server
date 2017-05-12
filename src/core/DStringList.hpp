#ifndef DSTRINGLIST_HPP
#define DSTRINGLIST_HPP

#include <list>
#include "DString.hpp"

using namespace std;

class DStringList : public list<DString>
{
public:
    explicit DStringList();
    ~DStringList();

    bool isEmpty() const;
    int length();

    DString &operator[](int i);
    const DString& operator[](int i) const;

    DStringList &operator<<(const DString &str);
    DStringList &operator<<(const DStringList &other);
    DStringList &operator=(const DStringList &other);

    DString &at(int pos);
    const DString& at(int pos) const;

    DString join(const DString &sep);

};

typedef DStringList::iterator DStringListItor;
typedef DStringList::const_iterator DStringListConstItor;

#endif // DSTRINGLIST_HPP
