#ifndef KERNEL_REQUEST_HPP
#define KERNEL_REQUEST_HPP

#include "DGlobal.hpp"
#include "DStringList.hpp"
#include "kernel_utility.hpp"

#include <map>
#include <string.h>

#define DEFAULT_VHOST   "__defaultVhost__"

class kernel_request
{
public:
    kernel_request();
    virtual ~kernel_request();

    void copy(kernel_request *src);

    void set_tcUrl(const DString &value);

    void set_stream(const DString &value);

    // /live/livestream.flv?vhost=test.com&arg1=temp&arg2=temp
    void set_http_url(const DString &_host, const DString &url, const DString &refer, bool del_suffix);

    DString get_stream_url();

public:
    DString vhost;
    DString app;
    DString stream;

    DString schema;
    DString host;
    DString port;
    std::map<DString,DString> params;
    DString oriParam;

    DString tcUrl;
    DString pageUrl;
    DString swfUrl;
};

#endif // KERNEL_REQUEST_HPP
