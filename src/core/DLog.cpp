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
    , m_log2Console(true)
    , m_log2File(false)
    , m_enablFILE(false)
    , m_enableLINE(false)
    , m_enableFUNTION(false)
    , m_enableColorPrint(true)
    , m_file(NULL)
    , m_timeFormat("yyyy-MM-dd hh:mm:ss.ms")
    , m_filePath(DEFAULT_LOG_FILEPATH)
{

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

    if (m_filePath != path) {
        DFree(m_file);
    }

    m_filePath = path;
}

void DLog::reopen()
{
    DSpinLocker locker(&m_mutex);

    if (!m_log2File) {
        return;
    }

    if (m_file) {
        DFree(m_file);
    }

    openLogFile();
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
    }

    int len = 4096;
    char buf[len];
    int size = 0;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    DString time = DDateTime::toString(m_timeFormat, tv);

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
    if (m_log2Console) {
        switch (level) {
        case DLogLevel::Warn:
            printf("\033[33m%s\033[0m", buf);
            break;
        case DLogLevel::Error:
            printf("\033[31m%s\033[0m", buf);
            break;
        default:
            printf("\033[0m%s", buf);
            break;
        }
    }

    // log to file
    if (m_log2File) {
        bool ret = true;

        if (!m_file) {
            ret = openLogFile();
        }

        if (ret) {
            if (m_file->write(buf, size) != size) {
                fprintf(stderr, "write log data to file failed\n");
                DFree(m_file);
            }
        }
    }
}

bool DLog::openLogFile()
{
    DString dir = DFile::filePath(m_filePath);

    if (!DDir::exists(dir)) {
        if (!DDir::createDir(dir)) {
            fprintf(stderr, "create log dir %s failed\n", dir.c_str());
            return false;
        }
    }

    m_file = new DFile(m_filePath);

    if (!m_file->open("a+")) {
        fprintf(stderr, "open log file %s failed\n", m_filePath.c_str());
        DFree(m_file);
        return false;
    }

    m_file->setAutoFlush(true);

    return true;
}

