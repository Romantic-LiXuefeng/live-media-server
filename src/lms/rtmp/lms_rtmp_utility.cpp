#include "lms_rtmp_utility.hpp"
#include "lms_access_log.hpp"
#include "DDateTime.hpp"

void rtmp_access_log_begin(kernel_request *req, const DString &timestamp,
                           const DString &ip, const DString &md5, const DString &method, bool end)
{
    lms_access_log_info info;
    info.ip = ip;
    info.timestamp = timestamp;
    info.method = method;
    if (req) {
        info.referer = req->pageUrl;
        info.vhost = req->vhost;
        info.app = req->app;
        info.stream = req->stream;
        info.param = req->oriParam;
    }
    info.type = "rtmp";
    info.md5 = md5;

    info.number = DString::number(0);
    if (end) {
        info.number = DString::number(1);
    }

    lms_access_log::instance()->writeToFile(info);
}

void rtmp_access_log_end(kernel_request *req, const DString &ip, const DString &md5)
{
    DString timestamp = DDateTime::currentDate().toString("yyyy-MM-dd hh:mm:ss.ms");

    lms_access_log_info info;

    if (req) {
        info.vhost = req->vhost;
        info.app = req->app;
        info.stream = req->stream;
    }

    info.type = "rtmp";
    info.md5 = md5;
    info.ip = ip;
    info.timestamp = timestamp;
    info.number = DString::number(1);

    lms_access_log::instance()->writeToFile(info);
}
