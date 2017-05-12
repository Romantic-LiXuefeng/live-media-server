#include "lms_verify_refer.hpp"
#include "DRegExp.hpp"

lms_verify_refer::lms_verify_refer()
{

}

lms_verify_refer::~lms_verify_refer()
{

}

bool lms_verify_refer::check(rtmp_request *req, bool publish)
{
    lms_server_config_struct *config = lms_config::instance()->get_server(req);
    DAutoFree(lms_server_config_struct, config);
    if (!config) {
        return false;
    }

    if (!config->get_refer_enable(req)) {
        return true;
    }

    if (publish) {
        return check_publish(config, req, req->pageUrl);
    } else {
        return check_play(config, req, req->pageUrl);
    }
}

bool lms_verify_refer::check_publish(lms_server_config_struct *config, rtmp_request *req, const DString &url)
{
    bool ret = false;

    std::vector<DString> all = config->get_refer_all(req);
    for (int i = 0; i < (int)all.size(); ++i) {
        DRegExp reg(all.at(i));
        if (reg.execMatch(url)) {
            ret = true;
            break;
        }
    }

    std::vector<DString> publish = config->get_refer_publish(req);

    if (publish.empty()) {
        return ret;
    }
    ret = false;

    for (int i = 0; i < (int)publish.size(); ++i) {
        DRegExp reg(publish.at(i));
        if (reg.execMatch(url)) {
            ret = true;
            break;
        }
    }

    return ret;
}

bool lms_verify_refer::check_play(lms_server_config_struct *config, rtmp_request *req, const DString &url)
{
    bool ret = false;

    std::vector<DString> all = config->get_refer_all(req);
    for (int i = 0; i < (int)all.size(); ++i) {
        DRegExp reg(all.at(i));
        if (reg.execMatch(url)) {
            ret = true;
            break;
        }
    }

    std::vector<DString> play = config->get_refer_play(req);

    if (play.empty()) {
        return ret;
    }
    ret = false;

    for (int i = 0; i < (int)play.size(); ++i) {
        DRegExp reg(play.at(i));
        if (reg.execMatch(url)) {
            ret = true;
            break;
        }
    }

    return ret;
}
