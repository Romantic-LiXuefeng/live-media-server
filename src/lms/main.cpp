#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <getopt.h>
#include <vector>

#include "lms_threads_server.hpp"
#include "lms_config.hpp"
#include "lms_source.hpp"
#include "lms_global.hpp"

#include "kernel_log.hpp"
#include "DMemPool.hpp"
#include "DEvent.hpp"
#include "DTimer.hpp"
#include "DSignal.hpp"
#include "DStringList.hpp"
#include "DHostInfo.hpp"

#include <malloc.h>

#ifdef ENABLE_GPERF
#include <google/profiler.h>
#endif

std::vector<lms_threads_server*> servers;

kernel_context* global_context = new kernel_context();

void parse_options(int argc, char** argv)
{
    struct option long_options[] = {
        {"version", required_argument, 0, 'v'},
        {0, 0, 0, 0}
    };

    int opt;
    while((opt = getopt_long(argc, argv, "v", long_options, NULL)) != -1) {
        switch (opt)
        {
        case 'v':
            printf("%s\n", LMS_VERSION);
            exit(0);
        default:
            break;
        }
    }
}

void init_log()
{
    if (lms_config::instance()->get_log_enable_file()) {
        kernel_log::instance()->setEnableFILE(true);
    }

    if (lms_config::instance()->get_log_enable_function()) {
        kernel_log::instance()->setEnableFUNCTION(true);
    }

    if (lms_config::instance()->get_log_enable_line()) {
        kernel_log::instance()->setEnableLINE(true);
    }

    int level = lms_config::instance()->get_log_level();
    kernel_log::instance()->setLogLevel(level);

    DString path = lms_config::instance()->get_log_path();
    kernel_log::instance()->setFilePath(path);

    if (!lms_config::instance()->get_log_to_console()) {
        kernel_log::instance()->setLog2Console(false);
    }

    if (lms_config::instance()->get_log_to_file()) {
        kernel_log::instance()->setLog2File(true);
    }

    kernel_log::instance()->setEnableCache(false);
}

void onTimer()
{
    DMemPool::instance()->print();

    lms_source_manager::instance()->reset();
}

void onSignal()
{
    log_trace("SIGINT");

#ifdef ENABLE_GPERF
    log_trace("ProfilerStop");
    ProfilerStop();
#endif

    exit(0);
}

void onReload()
{
    log_trace("start reload......");

    // 解析配置文件
    lms_config::instance()->parse_file();

    init_log();

    lms_source_manager::instance()->reload();

    for (int i = 0; i < (int)servers.size(); ++i) {
        servers.at(i)->reload();
    }
}

void start_server()
{
    int worker_count = lms_config::instance()->get_worker_count();

    for (int i = 0; i < worker_count; ++i) {
        lms_threads_server *srv = new lms_threads_server();
        servers.push_back(srv);

        int ret = srv->start();
        if (ret != 0) {
            log_error("thread create and start failed");
            ::exit(-1);
        }
    }
}

void start_signal(DEvent *event)
{
    // 处理SIGINT信号
    DSignal *sig = new DSignal(event);
    sig->setHandler(onSignal);
    if (!sig->open(SIGINT)) {
        log_error("signal SIGINT open failed");
        ::exit(-1);
    }

    // 处理SIGHUP信号，用来reload
    DSignal *reload_sig = new DSignal(event);
    reload_sig->setHandler(onReload);
    if (!reload_sig->open(SIGHUP)) {
        log_error("signal SIGHUP open failed");
        ::exit(-1);
    }
}

void start_timer(DEvent *event)
{
    // 启动定时器
    DTimer *timer = new DTimer(event);
    timer->setTimerEvent(onTimer);
    if (!timer->start(1000)) {
        log_error("timer start failed");
        ::exit(-1);
    }
}

int main(int argc, char *argv[])
{
    // 解析参数
    parse_options(argc, argv);

    // 解析配置文件
    lms_config::instance()->parse_file();

    /**
     * 后台启动，1表示不改变程序的根目录，0表示将STDIN, STDOUT, STDERR都重定向到/dev/null
     */
    if (lms_config::instance()->get_daemon()) {
        ::daemon(1, 0);
    }

#ifdef ENABLE_GPERF
    ProfilerStart("lms.prof");
#endif

    bool mempool_enable = lms_config::instance()->get_mempool_enable();
    DMemPool::instance()->setEnable(mempool_enable);

    // 减少内存碎片
    mallopt(M_MMAP_THRESHOLD, 128*1024);

    // 忽略SIGPIPE信号
    ::signal(SIGPIPE, SIG_IGN);

    // 设置日志属性
    init_log();

    // 初始化epoll
    DEvent *event = new DEvent;

    // 启动信号监听
    start_signal(event);

    // 启动定时器
    start_timer(event);

    // 启动server
    start_server();

    event->start();

    return 0;
}
