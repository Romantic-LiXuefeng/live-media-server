#ifndef DDIR_HPP
#define DDIR_HPP

#include "DString.hpp"
#include "DStringList.hpp"

class DDir
{
public:
    DDir(const DString &dirName);
    virtual ~DDir();

public:
    /**
     * @brief 创建目录，可以创建多级目录
     */
    bool createDir();
    static bool createDir(const DString &dirName);

    /**
     * @brief 判断是不是目录
     */
    bool isDir();
    static bool isDir(const DString &dirName);

    /**
     * @brief 判断目录是否存在
     */
    bool exists();
    static bool exists(const DString &dirName);

    /**
     * @brief 删除目录，如果目录中有东西会清掉
     */
    bool remove();
    static bool remove(const DString &dirName);

    /**
     * @brief 清除目录中的内容
     */
    bool cleanDir();
    static bool cleanDir(const DString &dirName);

private:
    DString _dirName;
};

#endif // DDIR_HPP
