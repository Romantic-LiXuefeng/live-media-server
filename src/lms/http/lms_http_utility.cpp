#include "lms_http_utility.hpp"
#include "lms_access_log.hpp"
#include "DDateTime.hpp"

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

void http_access_log_begin(DHttpParser *parser, int status, const DString &timestamp,
                           const DString &ip, const DString &md5, bool end)
{
    kernel_request *req = new kernel_request();
    DAutoFree(kernel_request, req);

    DString _url = parser->getUrl();
    DString _host = parser->feild("Host");
    DString _refer = parser->feild("Referer");
    DString _agent = parser->feild("User-Agent");
    DString _method = parser->method();

    req->set_http_url(_host, _url, _refer, true);

    lms_access_log_info info;
    info.ip = ip;
    info.status = DString::number(status);
    info.timestamp = timestamp;
    info.agent = _agent;
    info.method = _method;
    info.referer = _refer;
    info.vhost = req->vhost;
    info.app = req->app;
    info.stream = req->stream;
    info.param = req->oriParam;
    info.type = "http";
    info.md5 = md5;

    info.number = DString::number(0);
    if (end) {
        info.number = DString::number(1);
    }

    lms_access_log::instance()->writeToFile(info);
}

void http_access_log_end(kernel_request *req, const DString &ip, const DString &md5)
{
    DString timestamp = DDateTime::currentDate().toString("yyyy-MM-dd hh:mm:ss.ms");

    lms_access_log_info info;

    if (req) {
        info.vhost = req->vhost;
        info.app = req->app;
        info.stream = req->stream;
    }
    info.type = "http";
    info.md5 = md5;
    info.ip = ip;
    info.timestamp = timestamp;
    info.number = DString::number(1);

    lms_access_log::instance()->writeToFile(info);
}
