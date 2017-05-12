#ifndef DSTRING_HPP
#define DSTRING_HPP

#include <string>
#include "DGlobal.hpp"

using namespace std;

typedef string::iterator DStringIterator;
class DStringList;

class DString : public string
{
public:
    DString();
    DString(const char *str);
    DString(const char *str, int n);
    DString(const string &str);
    DString(size_t n, char c);

    virtual ~DString();

    string toStdString();

    /**
     * Example:
     *   DString str("12345");
     *   str.chop(3);        // returns  "12"
     */
    void chop(int n);

    /**
     * Example:
     *   DString str("12345");
     *   str.truncate(3);        // returns  "123"
     */
    void truncate(int position);
    /**
     * Example:
     *   DString str("123456789");
     *   str.truncate("6);        // returns  "12345"
     */
    void truncate(const DString &key);

    /**
     * remove ('\t', '\n', '\v', '\f', '\r', ' ') from front and behind
     * Example:
     *   DString str("\f\r hello world! \t\n\v");
     *   str.trimmed()       // returns  "hello world!"
     */
    DString trimmed() const;

    /**
     * Example:
     *   DString str;
     *   const char *buf = "hello world!";
     *   str.sprintf("%s", buf);         // returns   "hello world!"
     */
    DString &sprintf(const char *cformat, ...);

    bool contains(const DString &str) const;

    bool startWith(const DString &str) const;

    bool endWith(const DString &str) const;

    /**
     * Example:
     *   DString str("hello world!");
     *   str.replace("o", "v");  // returns  "hellv wvrld!"
     *   str.replace("o", "v", false);  // returns  "hellv world!"
     */
    DString &replace(const DString &before, const DString &after, bool replaceAll = true);

    int toInt(bool *ok = 0, int base = 10);
    short toShort(bool *ok = 0, int base = 10);
    DString toHex();
    dint64 toInt64(bool *ok = 0, int base = 10);
    duint64 toUint64(bool *ok = 0, int base = 10);
    double toDouble(bool *ok = 0);

    inline bool isEmpty() const { return empty(); }
    int size() const { return length(); }

    DStringList split(const DString &sep);

    /**
     * Example:
     *   DString str("before");
     *   str.prepend("123");      // returns "123before"
     */
    DString &prepend(const DString &str);
    DString &prepend(const char *str, int size);

    DString & operator <<(dint32 value);
    DString & operator <<(duint32 value);
    DString & operator <<(dint64 value);
    DString & operator <<(duint64 value);
    DString & operator <<(double value);
    DString & operator <<(const DString &str);

public:
    static DString number(dint32 n);
    static DString number(duint32 n);
    static DString number(duint64 n);
    static DString number(dint64 n);
    static DString number(double n);
};


#endif // DSTRING_HPP
