#ifndef LMS_HTTP_UTILITY_HPP
#define LMS_HTTP_UTILITY_HPP

#include "DHttpParser.hpp"
#include "rtmp_global.hpp"

enum HttpStreamType
{
    HttpStreamFlv = 0,
    HttpStreamTs
};

/**
 * @brief generate_http_request
 * @param parser
 * @param req
 * @param del_suffix /live/123.flv if true, stream=123. if false, stream=123.flv
 * @return
 */
bool generate_http_request(DHttpParser *parser, rtmp_request *req, bool del_suffix = true);

dint8 get_http_stream_type(DHttpParser *parser);

#endif // LMS_HTTP_UTILITY_HPP
