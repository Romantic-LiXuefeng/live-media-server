#ifndef LMS_VERIFY_REFER_HPP
#define LMS_VERIFY_REFER_HPP

#include "rtmp_global.hpp"
#include "lms_config.hpp"

class lms_verify_refer
{
public:
    lms_verify_refer();
    ~lms_verify_refer();

    bool check(rtmp_request *req, bool publish);

private:
    bool check_publish(lms_server_config_struct *config, rtmp_request *req, const DString &url);
    bool check_play(lms_server_config_struct *config, rtmp_request *req, const DString &url);

};

#endif // LMS_VERIFY_REFER_HPP
