#ifndef LMS_HTTP_UTILITY_HPP
#define LMS_HTTP_UTILITY_HPP

#include "DHttpParser.hpp"
#include "DGlobal.hpp"
#include "kernel_request.hpp"

namespace HttpType {
enum OperateType
{
    Default = 0,
    FlvLive,
    TsLive,
    FlvRecv,
    TsRecv,
    SendFile
};

}

kernel_request *get_http_request(DHttpParser *parser, bool del_suffix = true);

void http_access_log_begin(DHttpParser *parser, int status, const DString &timestamp,
                           const DString &ip, const DString &md5, bool end = false);

void http_access_log_end(kernel_request *req, const DString &ip, const DString &md5);

#endif // LMS_HTTP_UTILITY_HPP
