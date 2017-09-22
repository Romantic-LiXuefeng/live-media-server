#ifndef LMS_HTTP_SEND_FILE_HPP
#define LMS_HTTP_SEND_FILE_HPP

#include "DSendFile.hpp"
#include "DHttpParser.hpp"
#include "kernel_request.hpp"

class lms_http_server_conn;

class lms_http_send_file
{
public:
    lms_http_send_file(lms_http_server_conn *conn);
    virtual ~lms_http_send_file();

    int initialize(DHttpParser *parser);
    int start();

    int flush();
    bool eof();

    bool reload();
    void release();

private:
    void get_config_value();
    int response_http_header();

private:
    lms_http_server_conn *m_conn;
    kernel_request *m_req;
    DSendFile *m_sendfile;

    bool m_enable;
    dint64 m_timeout;
    DString m_root;

    DString m_filePath;

};

#endif // LMS_HTTP_SEND_FILE_HPP
