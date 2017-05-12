#ifndef DSENDFILE_HPP
#define DSENDFILE_HPP

#include "DString.hpp"

class DSendFile
{
public:
    DSendFile(int sockfd);
    virtual ~DSendFile();

    int open(const DString &filename);
    void close();

    int sendFile();

    bool eof();

    dint64 getFileSize();

public:
    int m_sockfd;
    int m_filefd;

    off_t m_offset;
    dint64 m_count;
    dint64 m_filesize;
};

#endif // DSENDFILE_HPP
