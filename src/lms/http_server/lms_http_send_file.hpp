#ifndef LMS_HTTP_SEND_FILE_HPP
#define LMS_HTTP_SEND_FILE_HPP

#include "DHttpParser.hpp"
#include "DHttpHeader.hpp"
#include "DSendFile.hpp"
#include "lms_conn_base.hpp"
#include "rtmp_global.hpp"

/**
 * @brief The lms_http_send_file class  发送文件时，chunked配置无效
 */
class lms_http_send_file
{
public:
    lms_http_send_file(lms_conn_base *parent);
    virtual ~lms_http_send_file();

public:
    int initialize(DHttpParser *parser);
    int start();

    int sendFile();

    void release();

private:
    int response_header(int size);
    bool get_config_value(rtmp_request *req);

private:
    lms_conn_base *m_parent;
    DSendFile *m_sendfile;
    rtmp_request *m_req;

    dint64 m_timeout;
    DString m_root;

    DString m_filePath;
};

#endif // LMS_HTTP_SEND_FILE_HPP
