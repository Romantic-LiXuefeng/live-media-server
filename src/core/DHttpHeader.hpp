#ifndef DHTTPHEADER_HPP
#define DHTTPHEADER_HPP

#include <map>
#include "DString.hpp"

class DHttpHeader
{
public:
    DHttpHeader();
    ~DHttpHeader();

    void addValue(const DString &key, const DString &value);

    void setContentLength(int len);
    void setContentType(const DString &type);
    void setHost(const DString &host);
    void setServer(const DString &server);
    void setConnectionClose();
    void setConnectionKeepAlive();
    void setDate();

    /**
     * @brief getRequestString
     * @param type GET/POST
     * @param url  /test/1.flv
     */
    DString getRequestString(const DString &type, const DString &url);
    /**
     * @brief getResponseString
     * @param err 200/302
     * @param info OK/Found
     */
    DString getResponseString(int err, const DString &info);

    DString getContentType(const DString &fileType);

private:
    DString generateDate();

private:
    std::map<DString, DString> m_headers;

    static std::map<DString, DString> m_mime;
};

#endif // DHTTPHEADER_HPP
