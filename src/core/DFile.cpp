#include "DFile.hpp"

#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include "DStringList.hpp"

DFile::DFile(const DString &_m_fileName)
    : m_fileName(_m_fileName)
    , fp(NULL)
    , m_autoFlush(false)
{
}

DFile::~DFile()
{
    close();
}

bool DFile::atEnd()
{
    return pos() == size();
}

DString DFile::fileName()
{
    return m_fileName;
}

DString DFile::filePath()
{
    return filePath(m_fileName);
}

DString DFile::filePath(const DString &path)
{
    DString temp = path;
    DStringList args = temp.split("/");
    if (args.size() > 0) {
        args.remove(args.back());
    }

    DString ret = args.join("/");
    if (temp.startWith("/")) {
        ret.prepend("/");
    }

    return ret;
}

bool DFile::open(const DString &mode)
{
    if (fp) return true;

    fp = ::fopen(m_fileName.c_str(), mode.c_str());
    if (!fp) {
        return false;
    }

    return true;
}

bool DFile::isOpen()
{
    return (fp);
}

void DFile::close()
{
    if (fp) {
        ::fclose(fp);
        fp = NULL;
    }
}

dint64 DFile::size()
{
    struct stat buf;
    if (::stat(m_fileName.c_str(), &buf) == -1) {
        return -1;
    }

    return (dint64)buf.st_size;
}

int DFile::seek(dint64 pos)
{
    if (pos > size()) {
        return -1;
    }

    ::fseek(fp, pos, SEEK_SET);

    return ::ftell(fp);
}

dint64 DFile::pos()
{
    dint64 currentPos = ::ftell(fp);

    return currentPos;
}

void DFile::setAutoFlush(bool enabled)
{
    m_autoFlush = enabled;
}

bool DFile::flush()
{
    // fd is not buffered device
    int ret = ::fflush(fp);
    if (ret == EOF) {
        return false;
    }

    return  true;
}

dint64 DFile::read(char *data, dint64 maxSize)
{
    duint64 lastPos = pos();

    int readCnt = ::fread((void *)data, maxSize, 1, fp);
    if (readCnt < 1) {
        // check if reach end-of-file
        if (::feof(fp)) {
            return size() - lastPos;
        }
        if (::ferror(fp)) {
            ::clearerr(fp);
            return -1;
        }
    }

    return maxSize;
}

DString DFile::read(dint64 maxSize)
{
    DString array;
    char *buffer = new char[maxSize];
    DAutoFreeArray(char, buffer);

    int bytes = read(buffer, maxSize);
    if (bytes == -1) {
        return array;
    }

    if (bytes > 0) {
        array.append(buffer, bytes);
    }

    return array;
}

DString DFile::readAll()
{
    DString array;
    int count = 1024 * 128;

    while (true) {
        DString bytes = read(count);
        if (!bytes.empty()) {
            array.append(bytes);
            if (bytes.size() < count) {  // reach end-of-file
                break;
            }
        } else {
            break;
        }
    }

    return array;
}

dint64 DFile::readLine(char *data, dint64 maxSize)
{
    char *ret = ::fgets(data, maxSize, fp);
    if (!ret) {
        if (::feof(fp)) {
            return 0;
        } else {
            return -1;
        }
    }

    return ::strlen(data);
}

DString DFile::readLine(dint64 maxSize)
{
    DString array;
    char *buffer = new char[maxSize];
    DAutoFreeArray(char, buffer);
    if (readLine(buffer, maxSize) == -1) {
        return array;
    }
    array.append(buffer);

    return array;
}

dint64 DFile::write(const char *data, dint64 maxSize)
{
    dint64 ret = 0;
    int nitem = ::fwrite(data, maxSize, 1, fp);
    if (nitem < 1) {
        return -1;
    }

    if (m_autoFlush) {
        bool ret = flush();
        if (!ret) {
            return -2;
        }
    }

    if (nitem == 1) {
        ret = maxSize;
    }

    return ret;
}

dint64 DFile::write(const char *data)
{
    return write(data, strlen(data));
}

dint64 DFile::write(const DString &byteArray)
{
    return write(byteArray.data(), byteArray.size());
}

bool DFile::isFile()
{
    return DFile::isFile(m_fileName);
}

bool DFile::isFile(const DString &filePath)
{
    struct stat buf;
    if (::stat(filePath.c_str(), &buf) == -1) {
        return false;
    }

    if (S_ISREG(buf.st_mode)) {
        return true;
    }

    return false;
}

bool DFile::exists()
{
    return DFile::exists(m_fileName);
}

bool DFile::exists(const DString &filePath)
{
    struct stat buf;
    if (::stat(filePath.c_str(), &buf) == -1) {
        return false;
    }

    if (S_ISREG(buf.st_mode)) {
        return true;
    }

    return false;
}

bool DFile::remove()
{
    return DFile::remove(m_fileName);
}

bool DFile::remove(const DString &filePath)
{
    if (!DFile::exists(filePath)) {
        return false;
    }

    if (::unlink(filePath.c_str()) == -1) {
        return false;
    }

    return true;
}

DString DFile::suffix()
{
    return DFile::suffix(m_fileName);
}

DString DFile::suffix(const DString &fileName)
{
    DString ret;

    // find the last "."
    DString temp = fileName;
    DStringList all = temp.split(".");

    if (all.size() == 0) {
        return ret;
    }

    if (all.size() == 1) {
        ret = all.at(0);
        return ret;
    }

    if (all.size() > 1) {
        ret = all.at(all.size() - 1);
        return ret;
    }

    return ret;
}

DString DFile::baseName()
{
    return DFile::baseName(m_fileName);
}

DString DFile::baseName(const DString &fileName)
{
    DString ret;

    // find the last "."
    DString temp = fileName;
    DStringList all = temp.split("/");

    if (all.size() == 0) {
        return ret;
    }

    if (all.size() == 1) {
        ret = all.at(0);
        return ret;
    }

    if (all.size() > 1) {
        ret = all.at(all.size() - 1);
        return ret;
    }

    return ret;
}


