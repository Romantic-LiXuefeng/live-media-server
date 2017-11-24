#ifndef LMS_ACCESS_LOG_HPP
#define LMS_ACCESS_LOG_HPP

#include "DString.hpp"
#include "DFile.hpp"
#include "DSpinLock.hpp"

#define DEFAULT_ACCESS_VALUE    "-"

typedef struct _lms_access_log_info
{
    DString ip;
    DString timestamp;
    // http | rtmp
    DString type;
    // http: GET | POST, rtmp: publish | play
    DString method;
    // 200 | 403 | 404
    DString status;

    DString vhost;
    DString app;
    DString stream;

    // 问号后面的部分
    DString param;

    DString referer;
    // http User-Agent
    DString agent;
    // 如果为0，说明结束了
    DString number;
    // md5(ip + time + fd)
    DString md5;
}lms_access_log_info;

class lms_access_log
{
public:
    lms_access_log();
    ~lms_access_log();

public:
    void writeToFile(const lms_access_log_info &info);

    void setPath(const DString &path);
    void setType(const DString &type);
    void setEnable(bool enable);

    static lms_access_log *instance();

    void reopen();

private:
    bool open_file();

private:
    static lms_access_log *m_instance;

    DFile *m_file;

    bool m_enable;
    DString m_path;
    DString m_type;

    DSpinLock m_mutex;

};

#endif // LMS_ACCESS_LOG_HPP
