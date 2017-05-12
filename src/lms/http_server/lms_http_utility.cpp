#include "lms_http_utility.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"

bool generate_http_request(DHttpParser *parser, rtmp_request *req, bool del_suffix)
{
    DString _url = parser->getUrl();
    DString _host = parser->feild("Host");
    DString _refer = parser->feild("Referer");

    req->set_http_url(_host, _url, _refer, del_suffix);

    if (req->vhost.isEmpty() || req->stream.isEmpty()) {
        return false;
    }

    return true;
}

dint8 get_http_stream_type(DHttpParser *parser)
{
    dint8 type;

    DString url = parser->getUrl();
    size_t pos = std::string::npos;

    if ((pos = url.find("?")) != std::string::npos) {
        url = url.substr(0, pos);
    }

    if (url.endWith(".flv")) {
        type = HttpStreamFlv;
    } else if (url.endWith(".ts")) {
        type = HttpStreamTs;
    }

    return type;
}
