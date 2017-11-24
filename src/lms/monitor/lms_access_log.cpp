#include "lms_access_log.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"
#include "lms_config.hpp"
#include "DDir.hpp"

lms_access_log* lms_access_log::m_instance = new lms_access_log;

lms_access_log::lms_access_log()
    : m_file(NULL)
    , m_enable(false)
    , m_path(DEFAULT_ACCESS_LOGPATH)
    , m_type("all")
{

}

lms_access_log::~lms_access_log()
{
    DFree(m_file);
}

void lms_access_log::writeToFile(const lms_access_log_info &info)
{
    DSpinLocker locker(&m_mutex);

    if (!m_enable) {
        DFree(m_file);
        return;
    }

    if (m_type != "all" && m_type != info.type) {
        return;
    }

    DString data = info.ip;
    data += " ";
    data += info.number;
    data += " ";
    data += "[" + info.timestamp + "]";
    data += " ";
    data += info.type;
    data += " ";
    if (info.method.isEmpty()) {
        data += "-";
    } else {
        data += info.method;
    }

    data += " ";
    if (info.status.isEmpty()) {
        data += "-";
    } else {
        data += info.status;
    }

    data += " ";
    data += info.md5;
    data += " ";
    if (info.vhost.isEmpty()) {
        data += "-";
    } else {
        data += info.vhost;
    }

    data += " ";
    if (info.app.isEmpty()) {
        data += "-";
    } else {
        data += info.app;
    }

    data += " ";
    if (info.stream.isEmpty()) {
        data += "-";
    } else {
        data += info.stream;
    }

    data += " ";
    if (info.referer.isEmpty()) {
        data += "\"-\"";
    } else {
        data += "\"" + info.referer + "\"";
    }

    data += " ";
    if (info.param.isEmpty()) {
        data += "\"-\"";
    } else {
        data += "\"" + info.param + "\"";
    }
    data += " ";
    if (info.agent.isEmpty()) {
        data += "\"-\"";
    } else {
        data += "\"" + info.agent + "\"";
    }
    data += "\n";

    bool ret = true;

    if (!m_file) {
        ret = open_file();
    }

    if (ret) {
        if (m_file->write(data) != data.size()) {
            log_error("write access log data to file failed\n");
            DFree(m_file);
        }
    }
}

void lms_access_log::setPath(const DString &path)
{
    DSpinLocker locker(&m_mutex);

    if (m_path != path) {
        DFree(m_file);
    }

    m_path = path;
}

void lms_access_log::setType(const DString &type)
{
    DSpinLocker locker(&m_mutex);

    m_type = type;
}

void lms_access_log::setEnable(bool enable)
{
    DSpinLocker locker(&m_mutex);

    m_enable = enable;
}

lms_access_log *lms_access_log::instance()
{
    return m_instance;
}

void lms_access_log::reopen()
{
    DSpinLocker locker(&m_mutex);

    if (!m_enable) {
        return;
    }

    if (m_file) {
        DFree(m_file);
    }

    open_file();
}

bool lms_access_log::open_file()
{
    DString dir = DFile::filePath(m_path);

    if (!DDir::exists(dir)) {
        if (!DDir::createDir(dir)) {
            log_error("create access log dir %s failed\n", dir.c_str());
            return false;
        }
    }

    m_file = new DFile(m_path);

    if (!m_file->open("a+")) {
        log_error("open access log file %s failed\n", m_path.c_str());
        DFree(m_file);
        return false;
    }

    m_file->setAutoFlush(true);

    return true;
}
