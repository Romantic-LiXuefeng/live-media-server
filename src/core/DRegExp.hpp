#ifndef DREGEXP_HPP
#define DREGEXP_HPP

#include "DStringList.hpp"
#include "pcre.h"

class DRegExp
{
public:
    DRegExp();
    /**
     * @brief DRegExp
     * @param pattern 正则表达式
     * @param caseless 忽略大小写
     */
    DRegExp(const DString &pattern, bool caseless = false);
    ~DRegExp();

public:
    void setCaseless(bool caseless);
    inline bool Caseless() { return m_caseless; }

    void setPattern(const DString &pattern);
    inline DString Pattern() { return m_pattern; }

    bool execMatch(const DString &str);

    DStringList capturedTexts() const;

public:
    pcre *m_regex;
    // 正则表达式
    DString m_pattern;
    // 待匹配的字符串
    DString m_str;
    // 忽略大小写
    bool m_caseless;

    DStringList m_captures;
};

#endif // DREGEXP_HPP
