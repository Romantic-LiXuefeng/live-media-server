#include "lms_http_utility.hpp"

kernel_request *get_http_request(DHttpParser *parser, bool del_suffix)
{
    kernel_request *req = new kernel_request();

    DString _url = parser->getUrl();
    DString _host = parser->feild("Host");
    DString _refer = parser->feild("Referer");

    req->set_http_url(_host, _url, _refer, del_suffix);

    if (req->vhost.isEmpty() || req->stream.isEmpty()) {
        DFree(req);
    }

    return req;
}
