#ifndef LMS_CONFIG_HPP
#define LMS_CONFIG_HPP

#include "DString.hpp"
#include "DSpinLock.hpp"
#include "DRegExp.hpp"
#include "lms_config_directive.hpp"
#include <vector>
#include <map>

class kernel_request;

class lms_config_base
{
public:
    lms_config_base() {}
    virtual ~lms_config_base() {}

    virtual void load_config(lms_config_directive *directive) = 0;
    virtual lms_config_base *copy() = 0;
};

class lms_hls_config_struct : public lms_config_base
{
public:
    lms_hls_config_struct();
    ~lms_hls_config_struct();

    virtual void load_config(lms_config_directive *directive);
    virtual lms_hls_config_struct *copy();

public:
    bool get_enable(bool &value);
    bool get_window(double &value);
    bool get_fragment(double &value);
    bool get_acodec(DString &value);
    bool get_vcodec(DString &value);
    bool get_m3u8_path(DString &value);
    bool get_ts_path(DString &value);
    bool get_time_jitter(bool &value);
    bool get_time_jitter_type(int &value);
    bool get_root(DString &value);
    bool get_time_expired(int &value);

public:
    bool exist_enable;
    // 默认false
    bool enable;

    bool exist_window;
    double window;

    bool exist_fragment;
    double fragment;

    bool exist_acodec;
    DString acodec;

    bool exist_vcodec;
    DString vcodec;

    bool exist_m3u8_path;
    DString m3u8_path;

    bool exist_ts_path;
    DString ts_path;

    bool exist_time_jitter;
    bool time_jitter;

    bool exist_jitter_type;
    // 默认middle
    int time_jitter_type;

    bool exist_root;
    DString root;

    bool exist_time_expired;
    int time_expired;

};

class lms_ts_codec_struct : public lms_config_base
{
public:
    lms_ts_codec_struct();
    ~lms_ts_codec_struct();

    virtual void load_config(lms_config_directive *directive);
    virtual lms_ts_codec_struct *copy();

public:
    bool get_acodec(DString &value);
    bool get_vcodec(DString &value);

public:
    bool exist_acodec;
    DString acodec;

    bool exist_vcodec;
    DString vcodec;
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

    bool get_proxy_ts_acodec(DString &value);
    bool get_proxy_ts_vcodec(DString &value);

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

    lms_ts_codec_struct *ts_codec;
};

/**
 * @brief The lms_live_config_struct class
 *
 * time_jitter          on;
 * time_jitter_type     simple | middle | high;
 * gop_cache            on;
 * fast_gop             on;
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
    bool get_in_ack_size(int &val);
    bool get_timeout(int &time);

public:
    bool exist_enable;
    // 默认true
    bool enable;

    bool exist_chunk_size;
    // 默认4096;
    int chunk_size;

    bool exist_in_ack_size;
    // 默认0
    int in_ack_size;

    bool exist_timeout;
    // 默认30，单位秒
    int timeout;
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
    bool get_timeout(int &time);
    bool get_flv_live_enable(bool &val);
    bool get_flv_recv_enable(bool &val);
    bool get_ts_live_enable(bool &val);
    bool get_ts_live_acodec(DString &val);
    bool get_ts_live_vcodec(DString &val);
    bool get_ts_recv_enable(bool &val);

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

    bool exist_timeout;
    // 默认30，单位秒
    int timeout;

    bool exist_flv_live_enable;
    // 默认false
    bool flv_live_enable;

    bool exist_flv_recv_enable;
    // 默认false
    bool flv_recv_enable;

    bool exist_ts_recv_enable;
    // 默认false
    bool ts_recv_enable;

    bool exist_ts_live_enable;
    bool ts_live_enable;

    lms_ts_codec_struct *ts_codec;
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

    bool get_matched(kernel_request *req);

public:
    bool get_rtmp_enable(bool &value);
    bool get_rtmp_chunk_size(int &value);
    bool get_rtmp_in_ack_size(int &value);
    bool get_rtmp_timeout(int &value);

    bool get_time_jitter(bool &value);
    bool get_time_jitter_type(int &value);
    bool get_gop_cache(bool &value);
    bool get_fast_gop(bool &value);
    bool get_queue_size(int &value);

    bool get_proxy_enable(bool &value);
    bool get_proxy_type(DString &value);
    bool get_proxy_pass(std::vector<DString> &value);
    bool get_proxy_vhost(DString &value);
    bool get_proxy_app(DString &value);
    bool get_proxy_stream(DString &value);
    bool get_proxy_timeout(int &value);
    bool get_proxy_ts_acodec(DString &value);
    bool get_proxy_ts_vcodec(DString &value);

    bool get_http_enable(bool &value);
    bool get_http_buffer_length(int &value);
    bool get_http_chunked(bool &value);
    bool get_http_root(DString &value);
    bool get_http_timeout(int &value);

    bool get_flv_live_enable(bool &value);
    bool get_flv_recv_enable(bool &value);
    bool get_ts_live_enable(bool &value);
    bool get_ts_live_acodec(DString &value);
    bool get_ts_live_vcodec(DString &value);
    bool get_ts_recv_enable(bool &value);

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

    bool get_hls_enable(bool &value);
    bool get_hls_window(double &value);
    bool get_hls_fragment(double &value);
    bool get_hls_acodec(DString &value);
    bool get_hls_vcodec(DString &value);
    bool get_hls_m3u8_path(DString &value);
    bool get_hls_ts_path(DString &value);
    bool get_hls_time_jitter(bool &value);
    bool get_hls_time_jitter_type(int &value);
    bool get_hls_root(DString &value);
    bool get_hls_time_expired(int &value);

public:
    DString type;
    DString pattern;

    lms_rtmp_config_struct *rtmp;
    lms_live_config_struct *live;
    lms_proxy_config_struct *proxy;
    lms_http_config_struct *http;
    lms_refer_config_struct *refer;
    lms_hook_config_struct *hook;
    lms_hls_config_struct *hls;
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

    bool get_matched(kernel_request *req);

public:
    bool get_rtmp_enable(kernel_request *req);
    int  get_rtmp_chunk_size(kernel_request *req);
    int  get_rtmp_in_ack_size(kernel_request *req);
    int  get_rtmp_timeout(kernel_request *req);

    bool get_time_jitter(kernel_request *req);
    int  get_time_jitter_type(kernel_request *req);
    bool get_gop_cache(kernel_request *req);
    bool get_fast_gop(kernel_request *req);
    int  get_queue_size(kernel_request *req);

    bool get_proxy_enable(kernel_request *req);
    DString get_proxy_type(kernel_request *req);
    std::vector<DString> get_proxy_pass(kernel_request *req);
    DString get_proxy_vhost(kernel_request *req);
    DString get_proxy_app(kernel_request *req);
    DString get_proxy_stream(kernel_request *req);
    int     get_proxy_timeout(kernel_request *req);
    DString get_proxy_ts_acodec(kernel_request *req);
    DString get_proxy_ts_vcodec(kernel_request *req);

    bool get_http_enable(kernel_request *req);
    int  get_http_buffer_length(kernel_request *req);
    bool get_http_chunked(kernel_request *req);
    DString get_http_root(kernel_request *req);
    int  get_http_timeout(kernel_request *req);

    bool get_flv_live_enable(kernel_request *req);
    bool get_flv_recv_enable(kernel_request *req);
    bool get_ts_live_enable(kernel_request *req);
    DString get_ts_live_acodec(kernel_request *req);
    DString get_ts_live_vcodec(kernel_request *req);
    bool get_ts_recv_enable(kernel_request *req);

    bool get_refer_enable(kernel_request *req);
    std::vector<DString> get_refer_all(kernel_request *req);
    std::vector<DString> get_refer_publish(kernel_request *req);
    std::vector<DString> get_refer_play(kernel_request *req);

    DString get_hook_rtmp_connect(kernel_request *req);
    DString get_hook_rtmp_connect_pattern(kernel_request *req);
    DString get_hook_rtmp_publish(kernel_request *req);
    DString get_hook_rtmp_publish_pattern(kernel_request *req);
    DString get_hook_rtmp_play(kernel_request *req);
    DString get_hook_rtmp_play_pattern(kernel_request *req);
    DString get_hook_rtmp_unpublish(kernel_request *req);
    DString get_hook_rtmp_stop(kernel_request *req);
    int     get_hook_timeout(kernel_request *req);

    bool get_hls_enable(kernel_request *req);
    double get_hls_window(kernel_request *req);
    double get_hls_fragment(kernel_request *req);
    DString get_hls_acodec(kernel_request *req);
    DString get_hls_vcodec(kernel_request *req);
    DString get_hls_m3u8_path(kernel_request *req);
    DString get_hls_ts_path(kernel_request *req);
    bool get_hls_time_jitter(kernel_request *req);
    int get_hls_time_jitter_type(kernel_request *req);
    DString get_hls_root(kernel_request *req);
    int get_hls_time_expired(kernel_request *req);

public:
    std::vector<DString> server_name;

    lms_rtmp_config_struct *rtmp;
    lms_live_config_struct *live;
    lms_proxy_config_struct *proxy;
    lms_http_config_struct *http;
    lms_refer_config_struct *refer;
    lms_hook_config_struct *hook;
    lms_hls_config_struct *hls;

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

    bool mempool_enable;      // 默认true

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
    int parse_file();

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

    bool get_mempool_enable();

    std::vector<int> get_rtmp_ports();
    std::vector<int> get_http_ports();

    lms_server_config_struct *get_server(kernel_request *req);

private:
    static lms_config *m_instance;

private:
    lms_config_struct *m_config;

    DSpinLock m_mutex;
};

#endif // LMS_CONFIG_HPP
