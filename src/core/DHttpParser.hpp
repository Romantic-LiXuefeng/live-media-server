#ifndef DHTTPPARSER_HPP
#define DHTTPPARSER_HPP

#include "http_parser.h"
#include "DString.hpp"
#include <map>

// for http parser macros
#define LMS_HTTP_METHOD_OPTIONS      HTTP_OPTIONS
#define LMS_HTTP_METHOD_GET          HTTP_GET
#define LMS_HTTP_METHOD_POST         HTTP_POST
#define LMS_HTTP_METHOD_PUT          HTTP_PUT
#define LMS_HTTP_METHOD_DELETE       HTTP_DELETE

class DHttpParser
{
public:
    // enum http_parser_type { HTTP_REQUEST, HTTP_RESPONSE, HTTP_BOTH };
    DHttpParser(enum http_parser_type type);
    ~DHttpParser();

public:
    int parse(const char *data, unsigned int length);

    bool completed();
    DString getUrl();
    DString feild(const DString &key);
    DString method();
    duint16 statusCode();

public:
    // functions start with _ should not be called in outside.
    static int onMessageBegin(http_parser* parser);
    static int onHeaderscComplete(http_parser* parser);
    static int onMessageComplete(http_parser* parser);
    static int onUrl(http_parser* parser, const char* at, size_t length);
    static int onHeaderField(http_parser* parser, const char* at, size_t length);
    static int onHeaderValue(http_parser* parser, const char* at, size_t length);
    static int onBody(http_parser* parser, const char* at, size_t length);

private:
    void init();

private:
    http_parser_settings m_settings;
    http_parser m_parser;

private:
    enum http_parser_type m_type;

    bool m_completed;
    DString m_url;

    bool m_lasWasValue;
    DString m_field_key;
    DString m_field_value;
    std::map<DString, DString> m_fields;
};

#endif // DHTTPPARSER_HPP
