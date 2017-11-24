#ifndef LMS_HTTP_PROCESS_BASE_HPP
#define LMS_HTTP_PROCESS_BASE_HPP

#include "kernel_request.hpp"
#include "kernel_global.hpp"
#include "DHttpParser.hpp"

class lms_http_process_base
{
public:
    lms_http_process_base();
    virtual ~lms_http_process_base();

public:
    virtual int initialize(DHttpParser *parser);
    virtual int start();

    virtual int flush();
    virtual bool eof();

    virtual bool reload();
    virtual void release();

    virtual kernel_request *request();

    virtual int process(CommonMessage *msg);
    virtual int service();
};

#endif // LMS_HTTP_PROCESS_BASE_HPP
