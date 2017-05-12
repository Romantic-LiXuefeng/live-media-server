#ifndef DFILE_HPP
#define DFILE_HPP

#include "DString.hpp"
#include "DGlobal.hpp"

class DFile
{
public:
    DFile(const DString &_fileName);
    ~DFile();

public:
    bool atEnd();
    DString fileName();
    DString filePath();
    static DString filePath(const DString &path);

    bool open(const DString &mode);
    bool isOpen();
    void close();

    dint64 size();
    int seek(dint64 pos);
    dint64 pos();

    void setAutoFlush(bool enabled);
    bool flush();

    dint64 read(char *data, dint64 maxSize);
    DString read(dint64 maxSize);
    DString readAll();

    dint64 readLine(char *data, dint64 maxSize);
    DString readLine(dint64 maxSize = 0);

    /**
    * @brief write data to file
    * @param @a data with @a size
    * @retval the actual size of writed.
    */
    dint64 write(const char *data, dint64 size);
    /**
    * @overload write()
    * @brief write string data to file
    */
    dint64 write(const char *data);
    /**
    * @overload write()
    * @brief write @a byteArray with byteArray.size() to file
    */
    dint64 write(const DString &byteArray);

    bool isFile();
    static bool isFile(const DString &filePath);

    bool exists();

    static bool exists(const DString &filePath);

    bool remove();
    static bool remove(const DString &filePath);

    /*!
        Returns the suffix of the file.
        The suffix consists of all characters in the file after (but not including) the last '.'.
        Example:

        "/tmp/archive.tar.gz" --->  gz;
     */
    DString suffix();
    static DString suffix(const DString &fileName);

    DString baseName();
    static DString baseName(const DString &fileName);

    /*!
        return the file handle.
    */
    inline FILE *handle() { return fp; }

private:
    DString m_fileName;
    FILE *fp;
    bool m_autoFlush;
};

#endif // DFILE_HPP
