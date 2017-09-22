#ifndef LMS_GLOBAL_HPP
#define LMS_GLOBAL_HPP

#include "kernel_global.hpp"
#include "rtmp_global.hpp"

#define LMS_INTERNAL_STR(v) #v
#define LMS_XSTR(v)         LMS_INTERNAL_STR(v)

#define VERSION_MAJOR       1
#define VERSION_MINOR       0
#define VERSION_REVISION    0

#define LMS_VERSION         LMS_XSTR(VERSION_MAJOR) "." LMS_XSTR(VERSION_MINOR) "." LMS_XSTR(VERSION_REVISION)

RtmpMessage *common_to_rtmp(CommonMessage *msg);
CommonMessage *rtmp_to_common(RtmpMessage *msg);

#endif // LMS_GLOBAL_HPP
