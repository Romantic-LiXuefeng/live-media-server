#include "DHttpParser.hpp"
#include <string.h>

DHttpParser::DHttpParser(enum http_parser_type type)
    : m_type(type)
    , m_lasWasValue(true)
{
    init();
}

DHttpParser::~DHttpParser()
{

}

void DHttpParser::init()
{
    memset(&m_settings, 0, sizeof(m_settings));
    m_settings.on_message_begin = onMessageBegin;
    m_settings.on_url = onUrl;
    m_settings.on_header_field = onHeaderField;
    m_settings.on_header_value = onHeaderValue;
    m_settings.on_headers_complete = onHeaderscComplete;
    m_settings.on_body = onBody;
    m_settings.on_message_complete = onMessageComplete;

    http_parser_init(&m_parser, m_type);
    m_parser.data = reinterpret_cast<void*>(this);
}

int DHttpParser::parse(const char *data, unsigned int length)
{
    int nparsed = http_parser_execute(&m_parser, &m_settings, data, length);

    if (m_parser.http_errno != 0) {
        return -1;
    }

    if (nparsed < 0) {
        return -2;
    }

    return 0;
}

bool DHttpParser::completed()
{
    return m_completed;
}

DString DHttpParser::getUrl()
{
    return m_url;
}

DString DHttpParser::feild(const DString &key)
{
    std::map<DString, DString>::iterator iter;
    iter = m_fields.find(key);
    if (iter != m_fields.end())
    {
        return m_fields[key];
    }

    return "";
}

DString DHttpParser::method()
{
    return http_method_str((enum http_method)m_parser.method);
}

duint16 DHttpParser::statusCode()
{
    return m_parser.status_code;
}

int DHttpParser::onMessageBegin(http_parser *parser)
{
    DHttpParser *obj = (DHttpParser*)parser->data;
    obj->m_completed = false;
    obj->m_lasWasValue = true;

    return 0;
}

int DHttpParser::onHeaderscComplete(http_parser *parser)
{
    DHttpParser *obj = (DHttpParser*)parser->data;
    obj->m_completed = true;

    if (!obj->m_field_key.empty()) {
        obj->m_fields[obj->m_field_key] = obj->m_field_value;
    }

    return 0;
}

int DHttpParser::onMessageComplete(http_parser *parser)
{
    return 0;
}

int DHttpParser::onUrl(http_parser *parser, const char *at, size_t length)
{
    DHttpParser *obj = (DHttpParser*)parser->data;
    if (length > 0) {
        obj->m_url = DString(at, (int)length);
    }

    return 0;
}

int DHttpParser::onHeaderField(http_parser *parser, const char *at, size_t length)
{
    DHttpParser *obj = (DHttpParser*)parser->data;

    if (obj->m_lasWasValue) {
        if (!obj->m_field_key.empty()) {
            obj->m_fields[obj->m_field_key] = obj->m_field_value;
        }

        obj->m_field_key.clear();
        obj->m_field_value.clear();
    }

    obj->m_field_key.append(at, length);
    obj->m_lasWasValue = false;

    return 0;
}

int DHttpParser::onHeaderValue(http_parser *parser, const char *at, size_t length)
{
    DHttpParser *obj = (DHttpParser*)parser->data;
    obj->m_field_value.append(at, length);
    obj->m_lasWasValue = true;

    return 0;
}

int DHttpParser::onBody(http_parser *parser, const char *at, size_t length)
{
    return 0;
}
