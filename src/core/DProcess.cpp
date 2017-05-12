#include "DProcess.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>

DProcess::DProcess()
{
}

DProcess::~DProcess()
{

}

bool DProcess::start(const DString &cmd, const DStringList &arguments)
{
    int pid = ::fork();
    if (pid < 0) {
        return false;
    }

    if (pid > 0) {
        int status = 0;
        if(::waitpid(pid, &status, 0) == -1) {
            return false;
        }
    }

    if (pid == 0) {
        for (int i = 0; i < ::sysconf(_SC_OPEN_MAX); i++) {
            if (i != STDIN_FILENO && i != STDOUT_FILENO && i != STDERR_FILENO) {
                ::close(i);
            }
        }

        execute(cmd, arguments);
    }

    return true;
}

bool DProcess::startDetached(const DString &cmd, const DStringList &arguments)
{
    int pid = ::fork();

    if (pid < 0) {
        return false;
    }

    if (pid > 0) {
        int status = 0;
        if(::waitpid(pid, &status, 0) == -1) {
            return false;
        }
    }

    if (pid == 0) {
        for (int i = 0; i < ::sysconf(_SC_OPEN_MAX); i++) {
            if (i != STDIN_FILENO && i != STDOUT_FILENO && i != STDERR_FILENO) {
                ::close(i);
            }
        }

        pid = ::fork();

        if (pid < 0) {
            ::exit(0);
        }

        if (pid > 0) {
            ::exit(0);
        }

        if (pid == 0) {
            execute(cmd, arguments);
        }
    }

    return true;
}

void DProcess::execute(const DString &cmd, const DStringList &arguments)
{
    if (arguments.isEmpty()) {
        int ret = ::execv(cmd.c_str(), NULL);
        ::exit(ret);
    } else {
        DStringList args = arguments;
        args.push_front(cmd);

        char *argv[args.size() + 1];
        for (unsigned int i = 0; i < args.size(); ++i) {
            argv[i] = const_cast<char*>(args.at(i).c_str());
        }

        // append '\0' to argv last
        argv[args.size()] = (char*)0;

        int ret = ::execv(cmd.c_str(), argv);
        ::exit(ret);
    }
}


