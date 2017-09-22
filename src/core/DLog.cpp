#include "DLog.hpp"

#include "DDateTime.hpp"
#include "DFile.hpp"
#include "DDir.hpp"
#include "DGlobal.hpp"

#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <inttypes.h>

DLog::DLog()
    : m_logLevel(DLogLevel::Error)
    , m_enableCache(false)
    , m_log2Console(true)
    , m_log2File(false)
    , m_enablFILE(false)
    , m_enableLINE(false)
    , m_enableFUNTION(false)
    , m_enableColorPrint(true)
    , m_file(NULL)
    , m_timeFormat("yyyy-MM-dd hh:mm:ss.ms")
{
    m_filePath = DEFAULT_LOG_FILEPATH;
}

DLog::~DLog()
{
    DFree(m_file);
}

void DLog::setLogLevel(int level)
{
    DSpinLocker locker(&m_mutex);
    m_logLevel = level;
}

void DLog::setEnableCache(bool enabled)
{
    DSpinLocker locker(&m_mutex);

    m_enableCache = enabled;

    if (m_file && m_file->isOpen()) {
        m_file->setAutoFlush(!m_enableCache);
    }
}

void DLog::setLog2Console(bool enabled)
{
    DSpinLocker locker(&m_mutex);
    m_log2Console = enabled;
}

void DLog::setLog2File(bool enabled)
{
    DSpinLocker locker(&m_mutex);

    m_log2File = enabled;

    if (!m_log2File) {
        DFree(m_file);
        return;
    }

    gettimeofday(&m_tv, NULL);

    if (!openLogFile()) {
        return;
    }
}

void DLog::setEnableFILE(bool enabled)
{
    DSpinLocker locker(&m_mutex);
    m_enablFILE = enabled;
}

void DLog::setEnableLINE(bool enabled)
{
    DSpinLocker locker(&m_mutex);
    m_enableLINE = enabled;
}

void DLog::setEnableFUNCTION(bool enabled)
{
    DSpinLocker locker(&m_mutex);
    m_enableFUNTION = enabled;
}

void DLog::setEnableColorPrint(bool enabled)
{
    DSpinLocker locker(&m_mutex);
    m_enableColorPrint = enabled;
}

void DLog::setTimeFormat(const DString &fmt)
{
    DSpinLocker locker(&m_mutex);
    m_timeFormat = fmt;
}

void DLog::setFilePath(const DString &path)
{
    DSpinLocker locker(&m_mutex);
    m_filePath = path;
}

void DLog::verbose(const char *file, duint16 line, const char *function
                     , duint64 id, const char *fmt, ...)
{
    DSpinLocker locker(&m_mutex);

    if (m_logLevel > DLogLevel::Verbose) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    log(DLogLevel::Verbose, file, line, function, id, fmt, ap);
    va_end(ap);
}

void DLog::info(const char *file, duint16 line, const char *function
                  , duint64 id, const char *fmt, ...)
{
    DSpinLocker locker(&m_mutex);

    if (m_logLevel > DLogLevel::Info) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    log(DLogLevel::Info, file, line, function, id, fmt, ap);
    va_end(ap);
}

void DLog::trace(const char *file, duint16 line, const char *function
                   , duint64 id, const char *fmt, ...)
{
    DSpinLocker locker(&m_mutex);

    if (m_logLevel > DLogLevel::Trace) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    log(DLogLevel::Trace, file, line, function, id, fmt, ap);
    va_end(ap);
}

void DLog::warn(const char *file, duint16 line, const char *function
                  , duint64 id, const char *fmt, ...)
{
    DSpinLocker locker(&m_mutex);

    if (m_logLevel > DLogLevel::Warn) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    log(DLogLevel::Warn, file, line, function, id, fmt, ap);
    va_end(ap);
}

void DLog::error(const char *file, duint16 line, const char *function
                   , duint64 id, const char *fmt, ...)
{
    DSpinLocker locker(&m_mutex);

    if (m_logLevel > DLogLevel::Error) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    log(DLogLevel::Error, file, line, function, id, fmt, ap);
    va_end(ap);
}

void DLog::log(int level, const char *file, duint16 line, const char *function, duint64 id, const char *fmt, va_list ap)
{
    const char *p;

    switch (level) {
    case DLogLevel::Verbose:
        p = "verbose";
        break;
    case DLogLevel::Info:
        p = "info";
        break;
    case DLogLevel::Trace:
        p = "trace";
        break;
    case DLogLevel::Warn:
        p = "warn";
        break;
    case DLogLevel::Error:
        p = "error";
        break;
    default:
        p = "default";
        break;
    }

    int len = 4096;
    char buf[len];
    int size = 0;

    gettimeofday(&m_tv, NULL);
    DString time = DDateTime::toString(m_timeFormat, m_tv);

    size += snprintf(buf + size, len - size, "[%s]", time.c_str());
    size += snprintf(buf + size, len - size, "[%s]", p);

    if (m_enablFILE) {
        size += snprintf(buf + size, len - size, "[%s]", file);
    }

    if (m_enableLINE) {
        size += snprintf(buf + size, len - size, "[%d]", line);
    }

    if (m_enableFUNTION) {
        size += snprintf(buf + size, len - size, "[%s]", function);
    }

    size += snprintf(buf + size, len - size, "[%"PRIu64 "]", id);

    size += snprintf(buf + size, len - size, " ");
    size += vsnprintf(buf + size, len - size, fmt, ap);

    if ((level == DLogLevel::Error) && errno != 0) {
        size += snprintf(buf + size, len - size, "(%s)", strerror(errno));
    }

    size += snprintf(buf + size, len - size, "\n");

    // log to console
    if (m_log2Console && m_enableColorPrint) {
        if (level <= DLogLevel::Trace) {
            printf("\033[0m%s", buf);
        } else if (level == DLogLevel::Warn) {
            printf("\033[33m%s\033[0m", buf);
        } else if (level == DLogLevel::Error){
            printf("\033[31m%s\033[0m", buf);
        }
    } else if (m_log2Console && !m_enableColorPrint) {
        printf("\033[0m%s", buf);
    }

    // log to file
    if (m_log2File) {
        if (!openLogFile()) {
            return;
        }

        m_file->write(buf, size);
    }
}

bool DLog::openLogFile()
{
    struct tm now = {0};
    localtime_r(&m_tv.tv_sec, &now);

    DString yyyy = DString().sprintf("%d",   1900 + now.tm_year);
    DString MM   = DString().sprintf("%02d", 1 + now.tm_mon);
    DString dd   = DString().sprintf("%02d", now.tm_mday);
    DString hh   = DString().sprintf("%02d", now.tm_hour);
    DString mm   = DString().sprintf("%02d", now.tm_min);

    DString path = m_filePath;

    path.replace("[yyyy]", yyyy, true);
    path.replace("[MM]", MM, true);
    path.replace("[dd]", dd, true);
    path.replace("[hh]", hh, true);
    path.replace("[mm]", mm, true);

    if (path == m_fileName && m_file && m_file->isOpen()) {
        return true;
    }

    m_fileName = path;

    DString dir = DFile::filePath(path);

    if (!DDir::exists(dir)) {
        if (!DDir::createDir(dir)) {
            fprintf(stderr, "create log dir %s failed.", dir.c_str());
            return false;
        }
    }

    DFree(m_file);
    m_file = new DFile(m_fileName);

    if (!m_file->open("a+")) {
        fprintf(stderr, "open log file %s failed", m_fileName.c_str());
        DFree(m_file);
        return false;
    }

    m_file->setAutoFlush(!m_enableCache);

    return true;
}

