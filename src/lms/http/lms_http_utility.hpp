#ifndef LMS_HTTP_UTILITY_HPP
#define LMS_HTTP_UTILITY_HPP

#include "DHttpParser.hpp"
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


#endif // LMS_HTTP_UTILITY_HPP
