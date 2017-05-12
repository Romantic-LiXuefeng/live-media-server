#include "DRegExp.hpp"

#include <string.h>

#define OVECCOUNT 30

DRegExp::DRegExp()
    : m_regex(NULL)
    , m_caseless(false)
{

}

DRegExp::DRegExp(const DString &pattern, bool caseless)
    : m_regex(NULL)
    , m_pattern(pattern)
    , m_caseless(caseless)
{

}

DRegExp::~DRegExp()
{
    if (m_regex) {
        pcre_free(m_regex);
        m_regex = NULL;
    }
}

void DRegExp::setCaseless(bool caseless)
{
    m_caseless = caseless;
}

void DRegExp::setPattern(const DString &pattern)
{
    m_pattern = pattern;
}

bool DRegExp::execMatch(const DString &str)
{
    bool ret = false;

    m_str = str;

    const char *error;
    int erroffset;

    if (m_caseless) {
        m_regex = pcre_compile(m_pattern.c_str(), PCRE_CASELESS, &error, &erroffset, NULL);
    } else {
        m_regex = pcre_compile(m_pattern.c_str(), 0, &error, &erroffset, NULL);
    }

    if (m_regex == NULL) {
        return ret;
    }

    int rc;
    char *p = const_cast<char*>(m_str.data());
    int len = m_str.length();
    int ovector[OVECCOUNT];

    while ((rc = pcre_exec(m_regex, NULL, p, len, 0, 0, ovector, OVECCOUNT)) != PCRE_ERROR_NOMATCH) {
        ret = true;

        for (int i = 0; i < rc; i++) {
            if (i == 0) {
                continue;
            }

            char *str_start = p + ovector[2 * i];
            int str_len = ovector[2 * i + 1] - ovector[2 * i];
            char matched[1024];
            memset(matched, 0, 1024);
            strncpy(matched, str_start, str_len);

            string str(str_start, str_len);
            m_captures.push_back(str);
        }

        p += ovector[1];
        if (!p) {
            break;
        }
    }

    return ret;
}

DStringList DRegExp::capturedTexts() const
{
    return m_captures;
}
