#include "DSendFile.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <errno.h>

DSendFile::DSendFile(int sockfd)
    : m_sockfd(sockfd)
    , m_filefd(-1)
    , m_offset(0)
    , m_count(0)
    , m_filesize(0)
{

}

DSendFile::~DSendFile()
{
    close();
}

int DSendFile::open(const DString &filename)
{
    mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH;

    m_filefd = ::open(filename.c_str(), O_RDONLY, mode);

    if (m_filefd == -1) {
        return -1;
    }

    struct stat buf;
    if (::stat(filename.c_str(), &buf) == -1) {
        return -2;
    }

    m_count = m_filesize = buf.st_size;

    return 0;
}

void DSendFile::close()
{
    if (m_filefd != -1) {
        ::close(m_filefd);
        m_filefd = -1;
    }
}

int DSendFile::sendFile()
{
    if (m_count <= 0) {
        return 0;
    }

    ssize_t nwrite = 0;

eintr:
    nwrite = sendfile(m_sockfd, m_filefd, &m_offset, m_count);

    if (nwrite == -1) {
        switch (errno) {
        case EAGAIN:
            return 0;
        case EINTR:
            goto eintr;
        default:
            return -1;
        }
    }

    if (nwrite == 0) {
        /*
         * if sendfile returns zero, then someone has truncated the file,
         * so the offset became beyond the end of the file
         */
        return -2;
    }

    m_count -= nwrite;

    return 0;
}

bool DSendFile::eof()
{
    return m_count <= 0;
}

dint64 DSendFile::getFileSize()
{
    return m_filesize;
}
