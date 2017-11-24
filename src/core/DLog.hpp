#ifndef DLOG_HPP
#define DLOG_HPP

#include "DString.hpp"
#include "DSpinLock.hpp"

#include <stdarg.h>

#define DEFAULT_LOG_FILEPATH        "../logs/error.log"

class DFile;

namespace DLogLevel {
    enum loglevel {
        Verbose = 0x01,
        Info = 0x02,
        Trace = 0x04,
        Warn = 0x08,
        Error = 0x0F
    };
}

class DLog
{
public:
    DLog();
    virtual ~DLog();

public:
    void setLogLevel(int level);

    void setLog2Console(bool enabled);
    void setLog2File(bool enabled);

    // 日志中打印文件名
    void setEnableFILE(bool enabled);
    // 日志中打印行数
    void setEnableLINE(bool enabled);
    // 日志中打印函数名
    void setEnableFUNCTION(bool enabled);
    // 在终端打印时是否加颜色
    void setEnableColorPrint(bool enabled);
    // 设置日志中时间的格式
    void setTimeFormat(const DString &fmt);
    // 设置文件名
    void setFilePath(const DString &path);

    void reopen();

public:
    virtual void verbose(const char *file, duint16 line, const char *function, duint64 id, const char* fmt, ...);
    virtual void info(const char *file, duint16 line, const char *function, duint64 id,  const char* fmt, ...);
    virtual void trace(const char *file, duint16 line, const char *function, duint64 id,  const char* fmt, ...);
    virtual void warn(const char *file, duint16 line, const char *function, duint64 id,  const char* fmt, ...);
    virtual void error(const char *file, duint16 line, const char *function, duint64 id,  const char* fmt, ...);

protected:
    virtual void log(int level, const char *file, duint16 line, const char *function, duint64 id, const char* fmt, va_list ap);

    bool openLogFile();

protected:
    int m_logLevel;
    bool m_log2Console;
    bool m_log2File;
    bool m_enablFILE;
    bool m_enableLINE;
    bool m_enableFUNTION;
    bool m_enableColorPrint;

    DFile *m_file;

    DString m_timeFormat;
    DString m_filePath;

    DSpinLock m_mutex;

};

#endif // DLOG_HPP
