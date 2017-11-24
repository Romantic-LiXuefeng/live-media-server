#include "lms_http_process_base.hpp"

lms_http_process_base::lms_http_process_base()
{

}

lms_http_process_base::~lms_http_process_base()
{

}

int lms_http_process_base::initialize(DHttpParser *parser)
{
    return 0;
}

int lms_http_process_base::start()
{
    return 0;
}

int lms_http_process_base::flush()
{
    return 0;
}

bool lms_http_process_base::eof()
{
    return true;
}

bool lms_http_process_base::reload()
{
    return true;
}

void lms_http_process_base::release()
{

}

kernel_request *lms_http_process_base::request()
{
    return NULL;
}

int lms_http_process_base::process(CommonMessage *msg)
{
    return 0;
}

int lms_http_process_base::service()
{
    return 0;
}
