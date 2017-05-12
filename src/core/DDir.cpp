#include "DDir.hpp"

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

bool isFile(const char *path);
bool isSpecialDir(const char *path);
void getFilePath(const char *path, const char *fileName,  char *filePath);
bool cleanPath(const char *path);

DDir::DDir(const DString &dirName)
    : _dirName(dirName)
{
    // process multi "//" to one "/"
    _dirName.push_back('/');
    _dirName.replace("//", "/");
}

DDir::~DDir()
{
}

bool DDir::createDir()
{
    return DDir::createDir(this->_dirName);
}

bool DDir::createDir(const DString &dirName)
{
    // process multi "//" to one "/"
    DString dn = dirName;
    dn.push_back('/');
    dn.replace("//", "/");

    int index = dn.startWith("/") ? 1 : 0;
    for (int i = index; i < dn.size(); ++i) {
        char &c = dn[i];

        if (c == '/') {
            c = '\0';
            int ret = ::access(dn.c_str(), F_OK);
            if (ret == 0) {
                c = '/';
                continue;
            }

            mode_t mode = S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IXOTH;
            ret = ::mkdir(dn.c_str(), mode);
            if (ret == 0) {
                c = '/';
                continue;
            }

            return false;
        }
    }

    return true;
}

bool DDir::isDir()
{
    return DDir::isDir(this->_dirName);
}

bool DDir::isDir(const DString &dirName)
{
    struct stat buf;
    if (::stat(dirName.c_str(), &buf) != 0) {
        return false;
    }

    if (S_ISDIR(buf.st_mode)) {
        return true;
    }

    return false;
}

bool DDir::exists()
{
    return DDir::exists(_dirName);
}

bool DDir::exists(const DString &dirName)
{
    struct stat buf;
    if (::stat(dirName.c_str(), &buf) != 0) {
        return false;
    }

    if (S_ISDIR(buf.st_mode)) {
        return true;
    }

    return false;
}

bool DDir::remove()
{
    return DDir::remove(this->_dirName);
}

bool DDir::remove(const DString &dirName)
{
    if (!cleanPath(dirName.c_str())) {
        return false;
    }

    if (::rmdir(dirName.c_str()) == 0) {
        return true;
    }

    return false;
}

bool DDir::cleanDir()
{
    return DDir::cleanDir(this->_dirName);
}

bool DDir::cleanDir(const DString &dirName)
{
    return cleanPath(dirName.c_str());
}

bool cleanPath(const char *path)
{
    DIR *dir;
    dirent *dirInfo;
    char filePath[PATH_MAX];

    if (isFile(path)) {
        if (::remove(path) != 0) {
            return false;
        }
    } else {
        if ((dir = ::opendir(path)) == NULL) {
            return false;
        }

        while ((dirInfo = readdir(dir)) != NULL) {
            getFilePath(path, dirInfo->d_name, filePath);
            if (isSpecialDir(dirInfo->d_name)) {
                continue;
            }

            if (!cleanPath(filePath)) {
                ::closedir(dir);
                return false;
            }
            ::rmdir(filePath);
        }

        ::closedir(dir);
    }

    return true;
}

bool isFile(const char *path)
{
    struct stat buf;
    if(::stat(path, &buf) != 0) {
        return false;
    }

    if (S_ISREG(buf.st_mode)) {
        return true;
    }

    return false;
}

bool isSpecialDir(const char *path)
{
    return strcmp(path, ".") == 0 || strcmp(path, "..") == 0;
}

void getFilePath(const char *path, const char *fileName, char *filePath)
{
    strcpy(filePath, path);

    if (filePath[strlen(path) - 1] != '/') {
        ::strcat(filePath, "/");
    }

    ::strcat(filePath, fileName);
}

