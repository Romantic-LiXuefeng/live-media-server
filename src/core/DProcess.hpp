#ifndef DPROCESS_HPP
#define DPROCESS_HPP

#include "DStringList.hpp"

class DProcess
{
public:
    DProcess();
    ~DProcess();

public:
    /**
     * @brief 阻塞方式启动进程
     */
    bool start(const DString &cmd, const DStringList &arguments);
    /**
     * @brief 非阻塞方式启动进程
     */
    bool startDetached(const DString &cmd, const DStringList &arguments);

private:
    void execute(const DString &cmd, const DStringList &arguments);

};

#endif // DPROCESS_HPP
