#ifndef LMS_RTMP_UTILITY_HPP
#define LMS_RTMP_UTILITY_HPP

#include "DString.hpp"
#include "kernel_request.hpp"

void rtmp_access_log_begin(kernel_request *req, const DString &timestamp,
                           const DString &ip, const DString &md5, const DString &method,
                           bool end = false);

void rtmp_access_log_end(kernel_request *req, const DString &ip, const DString &md5);


#endif // LMS_RTMP_UTILITY_HPP
