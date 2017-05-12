#ifndef LMS_CONFIG_HPP
#define LMS_CONFIG_HPP

#include "DString.hpp"
#include "DSpinLock.hpp"
#include "DRegExp.hpp"
#include "lms_config_directive.hpp"
#include <vector>
#include <map>

class rtmp_request;

class lms_config_base
{
public:
    lms_config_base() {}
    virtual ~lms_config_base() {}

    virtual void load_config(lms_config_directive *directive) = 0;
    virtual lms_config_base *copy() = 0;
};

class lms_proxy_config_struct : public lms_config_base
{
public:
    lms_proxy_config_struct();
    ~lms_proxy_config_struct();

    virtual void load_config(lms_config_directive *directive);
    virtual lms_proxy_config_struct *copy();

public:
    bool get_proxy_enable(bool &value);
    bool get_proxy_type(DString &value);
    bool get_proxy_pass(std::vector<DString> &value);
    bool get_proxy_vhost(DString &value);
    bool get_proxy_app(DString &value);
    bool get_proxy_stream(DString &value);
    bool get_proxy_timeout(int &value);

public:
    bool exist_enable;
    // 默认false
    bool enable;

    bool exist_type;
    // flv | ts | rtmp
    DString proxy_type;

    // 127.0.0.1:1935 127.0.0.1:80
    std::vector<DString> proxy_pass;

    bool exist_vhost;
    // 默认[vhost]，原始vhost
    DString proxy_vhost;

    bool exist_app;
    // 默认[app]，原始app
    DString proxy_app;

    bool exist_stream;
    // 默认[stream]，原始stream
    DString proxy_stream;

    bool exist_timeout;
    // 默认10秒
    int timeout;
};

/**
 * @brief The lms_live_config_struct class
 *
 * time_jitter          on;
 * time_jitter_type     simple | middle | high;
 * gop_cache            on;
 * fast_gop             on;
 * time_out             60;
 * queue_size           30; //单位M
 */
class lms_live_config_struct : public lms_config_base
{
public:
    lms_live_config_struct();
    ~lms_live_config_struct();

    virtual void load_config(lms_config_directive *directive);
    virtual lms_live_config_struct *copy();

public:
    bool get_jitter(bool &jitter);
    bool get_jitter_type(int &type);
    bool get_gop_cache(bool &gop);
    bool get_fast_gop(bool &gop);
    bool get_timeout(int &time);
    bool get_queue_size(int &size);

public:
    bool exist_jitter;
    // 默认true
    bool time_jitter;

    bool exist_jitter_type;
    // 默认middle
    int time_jitter_type;

    bool exist_gop_cache;
    // 默认true
    bool gop_cache;

    bool exist_fast_gop;
    // 默认false
    bool fast_gop;

    bool exist_timeout;
    // 默认30，单位秒
    int timeout;

    bool exist_queue_size;
    // 默认30Mb
    int queue_size;
};

/**
 * @brief lms_rtmp_config_struct class
 *
 * server {
 *     rtmp {
 *         enable               on;
 *         chunk_size           4096;
 *     }
 * }
 */
class lms_rtmp_config_struct : public lms_config_base
{
public:
    lms_rtmp_config_struct();
    ~lms_rtmp_config_struct();

    virtual void load_config(lms_config_directive *directive);
    virtual lms_rtmp_config_struct *copy();

    bool get_enable(bool &val);
    bool get_chunk_size(int &val);

public:
    bool exist_enable;
    // 默认true
    bool enable;

    bool exist_chunk_size;
    // 默认4096;
    int chunk_size;

};

/**
 * @brief lms_http_config_struct class
 *
 * server {
 *     http {
 *         enable               on;
 *         buffer_length        4096;
 *         chunked              on;
 *     }
 * }
 */
class lms_http_config_struct : public lms_config_base
{
public:
    lms_http_config_struct();
    ~lms_http_config_struct();

    virtual void load_config(lms_config_directive *directive);
    virtual lms_http_config_struct *copy();

    bool get_enable(bool &val);
    bool get_buffer_length(int &val);
    bool get_chunked(bool &val);
    bool get_root(DString &val);

public:
    bool exist_enable;
    // 默认true
    bool enable;

    bool exist_buffer_length;
    // 默认3000;
    int buffer_length;

    bool exist_chunked;
    // 默认true
    bool chunked;

    bool exist_root;

    DString root;
};

class lms_refer_config_struct : public lms_config_base
{
public:
    lms_refer_config_struct();
    ~lms_refer_config_struct();

    virtual void load_config(lms_config_directive *directive);
    virtual lms_refer_config_struct *copy();

    bool get_enable(bool &val);
    bool get_all(std::vector<DString> &value);
    bool get_publish(std::vector<DString> &value);
    bool get_play(std::vector<DString> &value);

public:
    bool exist_enable;
    // 默认false
    bool enable;

    std::vector<DString> all;
    std::vector<DString> publish;
    std::vector<DString> play;
};

class lms_hook_config_struct : public lms_config_base
{
public:
    lms_hook_config_struct();
    ~lms_hook_config_struct();

    virtual void load_config(lms_config_directive *directive);
    virtual lms_hook_config_struct *copy();

    bool get_connect_pattern(DString &value);
    bool get_connect(DString &value);

    bool get_publish_pattern(DString &value);
    bool get_publish(DString &value);

    bool get_play_pattern(DString &value);
    bool get_play(DString &value);

    bool get_unpublish(DString &value);
    bool get_stop(DString &value);

    bool get_timeout(int &time);

public:
    DString connect_pattern;
    DString connect;

    DString publish_pattern;
    DString publish;

    DString play_pattern;
    DString play;

    // publish => unpublish
    DString unpublish;

    // play => stop
    DString stop;

    bool exist_timeout;
    int timeout;
};

/**
 * @brief The lms_rtmp_location_config_struct class
 * location用来区分app/stream的配置
 *
 * location = live/stream {
 *     rtmp {
 *     }
 *     http {
 *     }
 *     hls {
 *     }
 * }
 */
class lms_location_config_struct : public lms_config_base
{
public:
    lms_location_config_struct();
    ~lms_location_config_struct();

    virtual void load_config(lms_config_directive *directive);
    virtual lms_location_config_struct *copy();

    bool get_matched(rtmp_request *req);

public:
    bool get_rtmp_enable(bool &value);
    bool get_rtmp_chunk_size(int &value);

    bool get_time_jitter(bool &value);
    bool get_time_jitter_type(int &value);
    bool get_gop_cache(bool &value);
    bool get_fast_gop(bool &value);
    bool get_timeout(int &value);
    bool get_queue_size(int &value);

    bool get_proxy_enable(bool &value);
    bool get_proxy_type(DString &value);
    bool get_proxy_pass(std::vector<DString> &value);
    bool get_proxy_vhost(DString &value);
    bool get_proxy_app(DString &value);
    bool get_proxy_stream(DString &value);
    bool get_proxy_timeout(int &value);

    bool get_http_enable(bool &value);
    bool get_http_buffer_length(int &value);
    bool get_http_chunked(bool &value);
    bool get_http_root(DString &value);

    bool get_refer_enable(bool &value);
    bool get_refer_all(std::vector<DString> &value);
    bool get_refer_publish(std::vector<DString> &value);
    bool get_refer_play(std::vector<DString> &value);

    bool get_hook_rtmp_connect(DString &value);
    bool get_hook_rtmp_connect_pattern(DString &value);
    bool get_hook_rtmp_publish(DString &value);
    bool get_hook_rtmp_publish_pattern(DString &value);
    bool get_hook_rtmp_play(DString &value);
    bool get_hook_rtmp_play_pattern(DString &value);
    bool get_hook_rtmp_unpublish(DString &value);
    bool get_hook_rtmp_stop(DString &value);
    bool get_hook_timeout(int &value);

public:
    DString type;
    DString pattern;

    lms_rtmp_config_struct *rtmp;
    lms_live_config_struct *live;
    lms_proxy_config_struct *proxy;
    lms_http_config_struct *http;
    lms_refer_config_struct *refer;
    lms_hook_config_struct *hook;
};

/**
 * @brief lms_server_config_struct class
 *
 * server {
 *     // 使用正则匹配，将.转换为\.
 *     server_name a.com b.com;
 *
 *     hls {
 *     }
 *
 *     http {
 *     }
 *
 *     rtmp {
 *     }
 *
 *     live {
 *     }
 *
 *     location = live/stream {
 *         rtmp {
 *         }
 *         hls {
 *         }
 *         live {
 *         }
 *         http {
 *         }
 *     }
 * }
 */
class lms_server_config_struct : public lms_config_base
{
public:
    lms_server_config_struct();
    ~lms_server_config_struct();

    virtual void load_config(lms_config_directive *directive);
    virtual lms_server_config_struct *copy();

    bool get_matched(rtmp_request *req);

public:
    bool get_rtmp_enable(rtmp_request *req);
    int  get_rtmp_chunk_size(rtmp_request *req);

    bool get_time_jitter(rtmp_request *req);
    int  get_time_jitter_type(rtmp_request *req);
    bool get_gop_cache(rtmp_request *req);
    bool get_fast_gop(rtmp_request *req);
    int  get_timeout(rtmp_request *req);
    int  get_queue_size(rtmp_request *req);

    bool get_proxy_enable(rtmp_request *req);
    DString get_proxy_type(rtmp_request *req);
    std::vector<DString> get_proxy_pass(rtmp_request *req);
    DString get_proxy_vhost(rtmp_request *req);
    DString get_proxy_app(rtmp_request *req);
    DString get_proxy_stream(rtmp_request *req);
    int     get_proxy_timeout(rtmp_request *req);

    bool get_http_enable(rtmp_request *req);
    int  get_http_buffer_length(rtmp_request *req);
    bool get_http_chunked(rtmp_request *req);
    DString get_http_root(rtmp_request *req);

    bool get_refer_enable(rtmp_request *req);
    std::vector<DString> get_refer_all(rtmp_request *req);
    std::vector<DString> get_refer_publish(rtmp_request *req);
    std::vector<DString> get_refer_play(rtmp_request *req);

    DString get_hook_rtmp_connect(rtmp_request *req);
    DString get_hook_rtmp_connect_pattern(rtmp_request *req);
    DString get_hook_rtmp_publish(rtmp_request *req);
    DString get_hook_rtmp_publish_pattern(rtmp_request *req);
    DString get_hook_rtmp_play(rtmp_request *req);
    DString get_hook_rtmp_play_pattern(rtmp_request *req);
    DString get_hook_rtmp_unpublish(rtmp_request *req);
    DString get_hook_rtmp_stop(rtmp_request *req);
    int     get_hook_timeout(rtmp_request *req);

public:
    std::vector<DString> server_name;

    lms_rtmp_config_struct *rtmp;
    lms_live_config_struct *live;
    lms_proxy_config_struct *proxy;
    lms_http_config_struct *http;
    lms_refer_config_struct *refer;
    lms_hook_config_struct *hook;

    std::vector<lms_location_config_struct*> locations;
};

/**
 * @brief lms_config_struct class
 * 保存全部配置内容
 *
 * 示例如下：
 * daemon   off;
 * log_level    error;
 *
 * server {
 *     http {
 *     }
 *
 *     rtmp {
 *     }
 * }
 * server {
 *     http {
 *     }
 *
 *     rtmp {
 *     }
 * }
 */
class lms_config_struct
{
public:
    lms_config_struct();
    ~lms_config_struct();

    void load_config(lms_config_directive *directive);

private:
    void load_global_config(lms_config_directive *directive);
    void load_server_config(lms_config_directive *directive);

public:
    bool daemon;
    int worker_count;

public:
/// log
    bool log_to_console;      // 日志是否打印在终端
    bool log_to_file;         // 日志是否写到文件中
    int  log_level;           // 日志级别
    DString log_path;         // 例如/logs/lms/[yyyy]/[MM]/[dd]/lms-[hh]-[mm].log，不支持秒级

    bool log_enable_line;     // 日志中打印文件的行数
    bool log_enable_function; // 日志中打印所属的函数
    bool log_enable_file;     // 日志中打印所属的文件名

    std::vector<int> rtmp_ports;
    std::vector<int> http_ports;

public:
    std::vector<lms_server_config_struct*> servers;

};

class lms_config
{
public:
    lms_config();
    ~lms_config();

    static lms_config *instance();

public:
    int parse_file(const DString &filename);

    void load_config(lms_config_directive *directive);

public:
    bool get_daemon();

    int get_worker_count();

    bool get_log_to_console();
    bool get_log_to_file();
    int get_log_level();
    DString get_log_path();
    bool get_log_enable_line();
    bool get_log_enable_function();
    bool get_log_enable_file();

    std::vector<int> get_rtmp_ports();
    std::vector<int> get_http_ports();

    lms_server_config_struct *get_server(rtmp_request *req);

private:
    static lms_config *m_instance;

private:
    lms_config_struct *m_config;

    DSpinLock m_mutex;
};

#endif // LMS_CONFIG_HPP
