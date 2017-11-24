#include "lms_config.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"
#include "lms_config_directive.hpp"
#include "kernel_request.hpp"
#include "lms_timestamp.hpp"

#include <algorithm>

#define RTMP_DEFAULT_LISTEN         1935

/*****************************************************************************/

lms_flv_dvr_config_struct::lms_flv_dvr_config_struct()
    : exist_enable(false)
    , enable(false)
    , exist_root(false)
    , exist_path(false)
    , exist_time_jitter(false)
    , exist_jitter_type(false)
    , exist_time_expired(false)
    , exist_fragment(false)
{

}

lms_flv_dvr_config_struct::~lms_flv_dvr_config_struct()
{

}

void lms_flv_dvr_config_struct::load_config(lms_config_directive *directive)
{
    if (true) {
        lms_config_directive *conf = directive->get("enable");
        if (conf && !conf->arg(0).isEmpty()) {
            if (conf->arg(0) == "on") {
                enable = true;
            }

            exist_enable = true;

            log_trace("enable=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("root");
        if (conf && !conf->arg(0).isEmpty()) {
            root = conf->arg(0);

            exist_root = true;

            log_trace("root=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("path");
        if (conf && !conf->arg(0).isEmpty()) {
            path = conf->arg(0);

            exist_path = true;

            log_trace("path=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("time_jitter");
        if (conf && !conf->arg(0).isEmpty()) {
            if (conf->arg(0) == "on") {
                time_jitter = true;
            }

            exist_time_jitter = true;

            log_trace("time_jitter=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("time_jitter_type");

        if (conf && !conf->arg(0).isEmpty()) {
            DString type = conf->arg(0);

            if (type == "simple") {
                time_jitter_type = LmsTimeStamp::simple;
            } else if (type == "middle") {
                time_jitter_type = LmsTimeStamp::middle;
            } else if (type == "high") {
                time_jitter_type = LmsTimeStamp::high;
            }

            exist_jitter_type = true;

            log_trace("time_jitter_type=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("fragment");

        if (conf && !conf->arg(0).isEmpty()) {
            fragment = conf->arg(0).toDouble();

            exist_fragment = true;

            log_trace("fragment=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("time_expired");

        if (conf && !conf->arg(0).isEmpty()) {
            time_expired = conf->arg(0).toInt();

            exist_time_expired = true;

            log_trace("time_expired=%s", conf->arg(0).c_str());
        }
    }
}

lms_flv_dvr_config_struct *lms_flv_dvr_config_struct::copy()
{
    lms_flv_dvr_config_struct *ret = new lms_flv_dvr_config_struct();
    ret->exist_enable = exist_enable;
    ret->enable = enable;
    ret->exist_fragment = exist_fragment;
    ret->fragment = fragment;
    ret->exist_path = exist_path;
    ret->path = path;
    ret->exist_time_jitter = exist_time_jitter;
    ret->time_jitter = time_jitter;
    ret->exist_jitter_type = exist_jitter_type;
    ret->time_jitter_type = time_jitter_type;
    ret->exist_root = exist_root;
    ret->root = root;
    ret->exist_time_expired = exist_time_expired;
    ret->time_expired = time_expired;

    return ret;
}

bool lms_flv_dvr_config_struct::get_enable(bool &value)
{
    if (exist_enable) {
        value = enable;
    }
    return exist_enable;
}

bool lms_flv_dvr_config_struct::get_fragment(double &value)
{
    if (exist_fragment) {
        value = fragment;
    }
    return exist_fragment;
}

bool lms_flv_dvr_config_struct::get_path(DString &value)
{
    if (exist_path) {
        value = path;
    }
    return exist_path;
}

bool lms_flv_dvr_config_struct::get_time_jitter(bool &value)
{
    if (exist_time_jitter) {
        value = time_jitter;
    }
    return exist_time_jitter;
}

bool lms_flv_dvr_config_struct::get_time_jitter_type(int &value)
{
    if (exist_jitter_type) {
        value = time_jitter_type;
    }
    return exist_jitter_type;
}

bool lms_flv_dvr_config_struct::get_root(DString &value)
{
    if (exist_root) {
        value = root;
    }
    return exist_root;
}

bool lms_flv_dvr_config_struct::get_time_expired(int &value)
{
    if (exist_time_expired) {
        value = time_expired;
    }
    return exist_time_expired;
}

/*****************************************************************************/

lms_hls_config_struct::lms_hls_config_struct()
    : exist_enable(false)
    , enable(false)
    , exist_window(false)
    , exist_fragment(false)
    , exist_acodec(false)
    , exist_vcodec(false)
    , exist_m3u8_path(false)
    , exist_ts_path(false)
    , exist_time_jitter(false)
    , time_jitter(false)
    , exist_jitter_type(false)
    , exist_root(false)
    , exist_time_expired(false)
{

}

lms_hls_config_struct::~lms_hls_config_struct()
{

}

void lms_hls_config_struct::load_config(lms_config_directive *directive)
{
    if (true) {
        lms_config_directive *conf = directive->get("enable");
        if (conf && !conf->arg(0).isEmpty()) {
            if (conf->arg(0) == "on") {
                enable = true;
            }

            exist_enable = true;

            log_trace("enable=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("window");

        if (conf && !conf->arg(0).isEmpty()) {
            window = conf->arg(0).toDouble();

            exist_window = true;

            log_trace("window=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("fragment");

        if (conf && !conf->arg(0).isEmpty()) {
            fragment = conf->arg(0).toDouble();

            exist_fragment = true;

            log_trace("fragment=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("acodec");
        if (conf && !conf->arg(0).isEmpty()) {
            acodec = conf->arg(0);

            exist_acodec = true;

            log_trace("exist_acodec=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("vcodec");
        if (conf && !conf->arg(0).isEmpty()) {
            vcodec = conf->arg(0);

            exist_vcodec = true;

            log_trace("exist_vcodec=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("m3u8_path");
        if (conf && !conf->arg(0).isEmpty()) {
            m3u8_path = conf->arg(0);

            exist_m3u8_path = true;

            log_trace("m3u8_path=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("ts_path");
        if (conf && !conf->arg(0).isEmpty()) {
            ts_path = conf->arg(0);

            exist_ts_path = true;

            log_trace("ts_path=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("time_jitter");
        if (conf && !conf->arg(0).isEmpty()) {
            if (conf->arg(0) == "on") {
                time_jitter = true;
            }

            exist_time_jitter = true;

            log_trace("time_jitter=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("time_jitter_type");

        if (conf && !conf->arg(0).isEmpty()) {
            DString type = conf->arg(0);

            if (type == "simple") {
                time_jitter_type = LmsTimeStamp::simple;
            } else if (type == "middle") {
                time_jitter_type = LmsTimeStamp::middle;
            } else if (type == "high") {
                time_jitter_type = LmsTimeStamp::high;
            }

            exist_jitter_type = true;

            log_trace("time_jitter_type=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("root");
        if (conf && !conf->arg(0).isEmpty()) {
            root = conf->arg(0);

            exist_root = true;

            log_trace("root=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("time_expired");

        if (conf && !conf->arg(0).isEmpty()) {
            time_expired = conf->arg(0).toInt();

            exist_time_expired = true;

            log_trace("time_expired=%s", conf->arg(0).c_str());
        }
    }
}

lms_hls_config_struct *lms_hls_config_struct::copy()
{
    lms_hls_config_struct *ret = new lms_hls_config_struct();
    ret->exist_enable = exist_enable;
    ret->enable = enable;
    ret->exist_window = exist_window;
    ret->window = window;
    ret->exist_fragment = exist_fragment;
    ret->fragment = fragment;
    ret->exist_acodec = exist_acodec;
    ret->acodec = acodec;
    ret->exist_vcodec = exist_vcodec;
    ret->vcodec = vcodec;
    ret->exist_m3u8_path = exist_m3u8_path;
    ret->m3u8_path = m3u8_path;
    ret->exist_ts_path = exist_ts_path;
    ret->ts_path = ts_path;
    ret->exist_time_jitter = exist_time_jitter;
    ret->time_jitter = time_jitter;
    ret->exist_jitter_type = exist_jitter_type;
    ret->time_jitter_type = time_jitter_type;
    ret->exist_root = exist_root;
    ret->root = root;
    ret->exist_time_expired = exist_time_expired;
    ret->time_expired = time_expired;

    return ret;
}

bool lms_hls_config_struct::get_enable(bool &value)
{
    if (exist_enable) {
        value = enable;
    }
    return exist_enable;
}

bool lms_hls_config_struct::get_window(double &value)
{
    if (exist_window) {
        value = window;
    }
    return exist_window;
}

bool lms_hls_config_struct::get_fragment(double &value)
{
    if (exist_fragment) {
        value = fragment;
    }
    return exist_fragment;

}

bool lms_hls_config_struct::get_acodec(DString &value)
{
    if (exist_acodec) {
        value = acodec;
    }
    return exist_acodec;
}

bool lms_hls_config_struct::get_vcodec(DString &value)
{
    if (exist_vcodec) {
        value = vcodec;
    }
    return exist_vcodec;
}

bool lms_hls_config_struct::get_m3u8_path(DString &value)
{
    if (exist_m3u8_path) {
        value = m3u8_path;
    }
    return exist_m3u8_path;
}

bool lms_hls_config_struct::get_ts_path(DString &value)
{
    if (exist_ts_path) {
        value = ts_path;
    }
    return exist_ts_path;
}

bool lms_hls_config_struct::get_time_jitter(bool &value)
{
    if (exist_time_jitter) {
        value = time_jitter;
    }
    return exist_time_jitter;
}

bool lms_hls_config_struct::get_time_jitter_type(int &value)
{
    if (exist_jitter_type) {
        value = time_jitter_type;
    }
    return exist_jitter_type;
}

bool lms_hls_config_struct::get_root(DString &value)
{
    if (exist_root) {
        value = root;
    }
    return exist_root;
}

bool lms_hls_config_struct::get_time_expired(int &value)
{
    if (exist_time_expired) {
        value = time_expired;
    }
    return exist_time_expired;
}

/*****************************************************************************/

lms_ts_codec_struct::lms_ts_codec_struct()
    : exist_acodec(false)
    , exist_vcodec(false)
{

}

lms_ts_codec_struct::~lms_ts_codec_struct()
{

}

void lms_ts_codec_struct::load_config(lms_config_directive *directive)
{
    if (true) {
        lms_config_directive *conf = directive->get("acodec");
        if (conf && !conf->arg(0).isEmpty()) {
            acodec = conf->arg(0);

            exist_acodec = true;

            log_trace("exist_acodec=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("vcodec");
        if (conf && !conf->arg(0).isEmpty()) {
            vcodec = conf->arg(0);

            exist_vcodec = true;

            log_trace("exist_vcodec=%s", conf->arg(0).c_str());
        }
    }
}

lms_ts_codec_struct *lms_ts_codec_struct::copy()
{
    lms_ts_codec_struct *ret = new lms_ts_codec_struct();
    ret->exist_acodec = exist_acodec;
    ret->acodec = acodec;
    ret->exist_vcodec = exist_vcodec;
    ret->vcodec = vcodec;

    return ret;
}

bool lms_ts_codec_struct::get_acodec(DString &value)
{
    if (exist_acodec) {
        value = acodec;
    }
    return exist_acodec;
}

bool lms_ts_codec_struct::get_vcodec(DString &value)
{
    if (exist_vcodec) {
        value = vcodec;
    }
    return exist_vcodec;
}

/*****************************************************************************/

lms_proxy_config_struct::lms_proxy_config_struct()
    : exist_enable(false)
    , enable(false)
    , exist_type(false)
    , exist_vhost(false)
    , exist_app(false)
    , exist_stream(false)
    , exist_timeout(false)
    , timeout(10)
    , ts_codec(NULL)
{

}

lms_proxy_config_struct::~lms_proxy_config_struct()
{
    DFree(ts_codec);
}

void lms_proxy_config_struct::load_config(lms_config_directive *directive)
{
    if (true) {
        lms_config_directive *conf = directive->get("ts_codec");
        if (conf) {
            ts_codec = new lms_ts_codec_struct();
            ts_codec->load_config(conf);
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("enable");
        if (conf && !conf->arg(0).isEmpty()) {
            if (conf->arg(0) == "on") {
                enable = true;
            }

            exist_enable = true;

            log_trace("enable=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("proxy_type");
        if (conf && !conf->arg(0).isEmpty()) {
            proxy_type = conf->arg(0);

            exist_type = true;

            log_trace("proxy_type=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("proxy_pass");
        if (conf) {
            for (int i = 0; i < (int)conf->args.size(); i++) {
                proxy_pass.push_back(conf->args.at(i));

                log_trace("proxy_pass=%s", conf->args.at(i).c_str());
            }
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("proxy_vhost");
        if (conf && !conf->arg(0).isEmpty()) {
            proxy_vhost = conf->arg(0);

            exist_vhost = true;

            log_trace("proxy_vhost=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("proxy_app");
        if (conf && !conf->arg(0).isEmpty()) {
            proxy_app = conf->arg(0);

            exist_app = true;

            log_trace("proxy_app=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("proxy_stream");
        if (conf && !conf->arg(0).isEmpty()) {
            proxy_stream = conf->arg(0);

            exist_stream = true;

            log_trace("proxy_stream=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("proxy_timeout");

        if (conf && !conf->arg(0).isEmpty()) {
            timeout = conf->arg(0).toInt();

            exist_timeout = true;

            log_trace("timeout=%s", conf->arg(0).c_str());
        }
    }
}

lms_proxy_config_struct *lms_proxy_config_struct::copy()
{
    lms_proxy_config_struct *proxy = new lms_proxy_config_struct();

    proxy->exist_enable = exist_enable;
    proxy->enable = enable;

    proxy->exist_type = exist_type;
    proxy->proxy_type = proxy_type;

    proxy->exist_vhost = exist_vhost;
    proxy->proxy_vhost = proxy_vhost;

    proxy->exist_app = exist_app;
    proxy->proxy_app = proxy_app;

    proxy->exist_stream = exist_stream;
    proxy->proxy_stream = proxy_stream;

    proxy->exist_timeout = exist_timeout;
    proxy->timeout = timeout;

    if (!proxy_pass.empty()) {
        proxy->proxy_pass.assign(proxy_pass.begin(), proxy_pass.end());
    }

    if (ts_codec) {
        proxy->ts_codec = ts_codec->copy();
    }

    return proxy;
}

bool lms_proxy_config_struct::get_proxy_enable(bool &value)
{
    if (exist_enable) {
        value = enable;
    }
    return exist_enable;
}

bool lms_proxy_config_struct::get_proxy_type(DString &value)
{
    if (exist_type) {
        value = proxy_type;
    }
    return exist_type;
}

bool lms_proxy_config_struct::get_proxy_pass(std::vector<DString> &value)
{
    if (!proxy_pass.empty()) {
        value.assign(proxy_pass.begin(), proxy_pass.end());
        return true;
    }
    return false;
}

bool lms_proxy_config_struct::get_proxy_vhost(DString &value)
{
    if (exist_vhost) {
        value = proxy_vhost;
    }
    return exist_vhost;
}

bool lms_proxy_config_struct::get_proxy_app(DString &value)
{
    if (exist_app) {
        value = proxy_app;
    }
    return exist_app;
}

bool lms_proxy_config_struct::get_proxy_stream(DString &value)
{
    if (exist_stream) {
        value = proxy_stream;
    }
    return exist_stream;
}

bool lms_proxy_config_struct::get_proxy_timeout(int &value)
{
    if (exist_timeout) {
        value = timeout;
    }
    return exist_timeout;
}

bool lms_proxy_config_struct::get_proxy_ts_acodec(DString &value)
{
    if (ts_codec) {
        return ts_codec->get_acodec(value);
    }

    return false;
}

bool lms_proxy_config_struct::get_proxy_ts_vcodec(DString &value)
{
    if (ts_codec) {
        return ts_codec->get_vcodec(value);
    }

    return false;
}

/*****************************************************************************/

lms_live_config_struct::lms_live_config_struct()
    : exist_jitter(false)
    , time_jitter(true)
    , exist_jitter_type(false)
    , time_jitter_type(LmsTimeStamp::middle)
    , exist_gop_cache(false)
    , gop_cache(true)
    , exist_fast_gop(false)
    , fast_gop(false)
    , exist_queue_size(false)
    , queue_size(30)
{

}

lms_live_config_struct::~lms_live_config_struct()
{

}

void lms_live_config_struct::load_config(lms_config_directive *directive)
{
    if (true) {
        lms_config_directive *conf = directive->get("time_jitter");

        if (conf && !conf->arg(0).isEmpty()) {
            if (conf->arg(0) == "off") {
                time_jitter = false;
            }
            exist_jitter = true;

            log_trace("time_jitter=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("time_jitter_type");

        if (conf && !conf->arg(0).isEmpty()) {
            DString type = conf->arg(0);

            if (type == "simple") {
                time_jitter_type = LmsTimeStamp::simple;
            } else if (type == "middle") {
                time_jitter_type = LmsTimeStamp::middle;
            } else if (type == "high") {
                time_jitter_type = LmsTimeStamp::high;
            }

            exist_jitter_type = true;

            log_trace("time_jitter_type=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("gop_cache");

        if (conf && !conf->arg(0).isEmpty()) {
            if (conf->arg(0) == "off") {
                gop_cache = false;
            }
            exist_gop_cache = true;

            log_trace("gop_cache=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("fast_gop");

        if (conf && !conf->arg(0).isEmpty()) {
            if (conf->arg(0) == "on") {
                fast_gop = true;
            }
            exist_fast_gop = true;

            log_trace("fast_gop=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("queue_size");

        if (conf && !conf->arg(0).isEmpty()) {
            queue_size = conf->arg(0).toInt();

            exist_queue_size = true;

            log_trace("exist_queue_size=%s", conf->arg(0).c_str());
        }
    }
}

lms_live_config_struct *lms_live_config_struct::copy()
{
    lms_live_config_struct *live = new lms_live_config_struct();

    live->exist_jitter = exist_jitter;
    live->time_jitter = time_jitter;
    live->exist_jitter_type = exist_jitter_type;
    live->time_jitter_type = time_jitter_type;
    live->exist_gop_cache = exist_gop_cache;
    live->gop_cache = gop_cache;
    live->exist_fast_gop = exist_fast_gop;
    live->fast_gop = fast_gop;
    live->exist_queue_size = exist_queue_size;
    live->queue_size = queue_size;

    return live;
}

bool lms_live_config_struct::get_jitter(bool &jitter)
{
    if (exist_jitter) {
        jitter = time_jitter;
    }
    return exist_jitter;
}

bool lms_live_config_struct::get_jitter_type(int &type)
{
    if (exist_jitter_type) {
        type = time_jitter_type;
    }
    return exist_jitter_type;
}

bool lms_live_config_struct::get_gop_cache(bool &gop)
{
    if (exist_gop_cache) {
        gop = gop_cache;
    }
    return exist_gop_cache;
}

bool lms_live_config_struct::get_fast_gop(bool &gop)
{
    if (exist_fast_gop) {
        gop = fast_gop;
    }
    return exist_fast_gop;
}

bool lms_live_config_struct::get_queue_size(int &size)
{
    if (exist_queue_size) {
        queue_size = size;
    }
    return exist_queue_size;
}

/*****************************************************************************/

lms_rtmp_config_struct::lms_rtmp_config_struct()
    : exist_enable(false)
    , enable(true)
    , exist_chunk_size(false)
    , chunk_size(4096)
    , exist_in_ack_size(false)
    , in_ack_size(0)
    , exist_timeout(false)
    , timeout(30)
{

}

lms_rtmp_config_struct::~lms_rtmp_config_struct()
{

}

void lms_rtmp_config_struct::load_config(lms_config_directive *directive)
{
    if (true) {
        lms_config_directive *conf = directive->get("enable");
        if (conf && !conf->arg(0).isEmpty()) {
            if (conf->arg(0) == "off") {
                enable = false;
            }

            exist_enable = true;

            log_trace("enable=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("chunk_size");
        if (conf && !conf->arg(0).isEmpty()) {
            chunk_size = conf->arg(0).toInt();

            exist_chunk_size = true;

            log_trace("chunk_size=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("in_ack_size");
        if (conf && !conf->arg(0).isEmpty()) {
            in_ack_size = conf->arg(0).toInt();

            exist_in_ack_size = true;

            log_trace("in_ack_size=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("timeout");

        if (conf && !conf->arg(0).isEmpty()) {
            timeout = conf->arg(0).toInt();

            exist_timeout = true;

            log_trace("timeout=%s", conf->arg(0).c_str());
        }
    }
}

lms_rtmp_config_struct *lms_rtmp_config_struct::copy()
{
    lms_rtmp_config_struct *rtmp = new lms_rtmp_config_struct();

    rtmp->exist_enable = exist_enable;
    rtmp->enable = enable;
    rtmp->exist_chunk_size = exist_chunk_size;
    rtmp->chunk_size = chunk_size;
    rtmp->exist_in_ack_size = exist_in_ack_size;
    rtmp->in_ack_size = in_ack_size;
    rtmp->exist_timeout = exist_timeout;
    rtmp->timeout = timeout;

    return rtmp;
}

bool lms_rtmp_config_struct::get_enable(bool &val)
{
    if (exist_enable) {
        val = enable;
    }
    return exist_enable;
}

bool lms_rtmp_config_struct::get_chunk_size(int &val)
{
    if (exist_chunk_size) {
        val = chunk_size;
    }
    return exist_chunk_size;
}

bool lms_rtmp_config_struct::get_in_ack_size(int &val)
{
    if (exist_in_ack_size) {
        val = in_ack_size;
    }
    return exist_in_ack_size;
}

bool lms_rtmp_config_struct::get_timeout(int &time)
{
    if (exist_timeout) {
        time = timeout;
    }
    return exist_timeout;
}

/*****************************************************************************/

lms_http_config_struct::lms_http_config_struct()
    : exist_enable(false)
    , enable(true)
    , exist_buffer_length(false)
    , buffer_length(3000)
    , exist_chunked(false)
    , chunked(true)
    , exist_root(false)
    , exist_timeout(false)
    , timeout(30)
    , exist_flv_live_enable(false)
    , flv_live_enable(false)
    , exist_flv_recv_enable(false)
    , flv_recv_enable(false)
    , exist_ts_recv_enable(false)
    , ts_recv_enable(false)
    , exist_ts_live_enable(false)
    , ts_live_enable(false)
    , ts_codec(NULL)
{

}

lms_http_config_struct::~lms_http_config_struct()
{
    DFree(ts_codec);
}

void lms_http_config_struct::load_config(lms_config_directive *directive)
{
    if (true) {
        lms_config_directive *conf = directive->get("ts_codec");
        if (conf) {
            ts_codec = new lms_ts_codec_struct();
            ts_codec->load_config(conf);
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("enable");
        if (conf && !conf->arg(0).isEmpty()) {
            if (conf->arg(0) == "off") {
                enable = false;
            }

            exist_enable = true;

            log_trace("enable=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("buffer_length");
        if (conf && !conf->arg(0).isEmpty()) {
            buffer_length = conf->arg(0).toInt();

            exist_buffer_length = true;

            log_trace("buffer_length=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("chunked");
        if (conf && !conf->arg(0).isEmpty()) {
            if (conf->arg(0) == "off") {
                chunked = false;
            }

            exist_chunked = true;

            log_trace("chunked=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("root");
        if (conf && !conf->arg(0).isEmpty()) {
            root = conf->arg(0);

            exist_root = true;

            log_trace("root=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("timeout");

        if (conf && !conf->arg(0).isEmpty()) {
            timeout = conf->arg(0).toInt();

            exist_timeout = true;

            log_trace("timeout=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("flv_live_enable");

        if (conf && !conf->arg(0).isEmpty()) {
            if (conf->arg(0) == "on") {
                flv_live_enable = true;
            }

            exist_flv_live_enable = true;
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("flv_recv_enable");

        if (conf && !conf->arg(0).isEmpty()) {
            if (conf->arg(0) == "on") {
                flv_recv_enable = true;
            }

            exist_flv_recv_enable = true;
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("ts_recv_enable");

        if (conf && !conf->arg(0).isEmpty()) {
            if (conf->arg(0) == "on") {
                ts_recv_enable = true;
            }

            exist_ts_recv_enable = true;
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("ts_live_enable");

        if (conf && !conf->arg(0).isEmpty()) {
            if (conf->arg(0) == "on") {
                ts_live_enable = true;
            }

            exist_ts_live_enable = true;
        }
    }
}

lms_http_config_struct *lms_http_config_struct::copy()
{
    lms_http_config_struct *http = new lms_http_config_struct();

    http->exist_enable = exist_enable;
    http->enable = enable;
    http->exist_buffer_length = exist_buffer_length;
    http->buffer_length = buffer_length;
    http->exist_chunked = exist_chunked;
    http->chunked = chunked;
    http->exist_root = exist_root;
    http->root = root;
    http->exist_timeout = exist_timeout;
    http->timeout = timeout;
    http->exist_flv_live_enable = exist_flv_live_enable;
    http->flv_live_enable = flv_live_enable;
    http->exist_flv_recv_enable = exist_flv_recv_enable;
    http->flv_recv_enable = flv_recv_enable;
    http->exist_ts_recv_enable = exist_ts_recv_enable;
    http->ts_recv_enable = ts_recv_enable;
    http->exist_ts_live_enable = exist_ts_live_enable;
    http->ts_live_enable = ts_live_enable;

    if (ts_codec) {
        http->ts_codec = ts_codec->copy();
    }

    return http;

}

bool lms_http_config_struct::get_enable(bool &val)
{
    if (exist_enable) {
        val = enable;
    }
    return exist_enable;
}

bool lms_http_config_struct::get_buffer_length(int &val)
{
    if (exist_buffer_length) {
        val = buffer_length;
    }
    return exist_buffer_length;
}

bool lms_http_config_struct::get_chunked(bool &val)
{
    if (exist_chunked) {
        val = chunked;
    }
    return exist_chunked;
}

bool lms_http_config_struct::get_root(DString &val)
{
    if (exist_root) {
        val = root;
    }
    return exist_root;
}

bool lms_http_config_struct::get_timeout(int &time)
{
    if (exist_timeout) {
        time = timeout;
    }
    return exist_timeout;
}

bool lms_http_config_struct::get_flv_live_enable(bool &val)
{
    if (exist_flv_live_enable) {
        val = flv_live_enable;
    }

    return exist_flv_live_enable;
}

bool lms_http_config_struct::get_flv_recv_enable(bool &val)
{
    if (exist_flv_recv_enable) {
        val = flv_recv_enable;
    }

    return exist_flv_recv_enable;
}

bool lms_http_config_struct::get_ts_live_enable(bool &val)
{
    if (exist_ts_live_enable) {
        val = ts_live_enable;
    }

    return exist_ts_live_enable;
}

bool lms_http_config_struct::get_ts_live_acodec(DString &val)
{
    if (ts_codec) {
        return ts_codec->get_acodec(val);
    }

    return false;
}

bool lms_http_config_struct::get_ts_live_vcodec(DString &val)
{
    if (ts_codec) {
        return ts_codec->get_vcodec(val);
    }

    return false;
}

bool lms_http_config_struct::get_ts_recv_enable(bool &val)
{
    if (exist_ts_recv_enable) {
        val = ts_recv_enable;
    }

    return exist_ts_recv_enable;
}

/*****************************************************************************/

lms_refer_config_struct::lms_refer_config_struct()
    : exist_enable(false)
    , enable(false)
{

}

lms_refer_config_struct::~lms_refer_config_struct()
{

}

void lms_refer_config_struct::load_config(lms_config_directive *directive)
{
    if (true) {
        lms_config_directive *conf = directive->get("enable");
        if (conf && !conf->arg(0).isEmpty()) {
            if (conf->arg(0) == "on") {
                enable = true;
            }

            exist_enable = true;

            log_trace("enable=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("all");
        if (conf) {
            for (int i = 0; i < (int)conf->args.size(); i++) {
                all.push_back(conf->args.at(i));

                log_trace("all=%s", conf->args.at(i).c_str());
            }
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("publish");
        if (conf) {
            for (int i = 0; i < (int)conf->args.size(); i++) {
                publish.push_back(conf->args.at(i));

                log_trace("publish=%s", conf->args.at(i).c_str());
            }
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("play");
        if (conf) {
            for (int i = 0; i < (int)conf->args.size(); i++) {
                play.push_back(conf->args.at(i));

                log_trace("play=%s", conf->args.at(i).c_str());
            }
        }
    }
}

lms_refer_config_struct *lms_refer_config_struct::copy()
{
    lms_refer_config_struct *refer = new lms_refer_config_struct();

    refer->exist_enable = exist_enable;
    refer->enable = enable;

    if (!all.empty()) {
        refer->all.assign(all.begin(), all.end());
    }

    if (!publish.empty()) {
        refer->publish.assign(publish.begin(), publish.end());
    }

    if (!play.empty()) {
        refer->play.assign(play.begin(), play.end());
    }

    return refer;
}

bool lms_refer_config_struct::get_enable(bool &val)
{
    if (exist_enable) {
        val = enable;
    }
    return exist_enable;
}

bool lms_refer_config_struct::get_all(std::vector<DString> &value)
{
    if (!all.empty()) {
        value.assign(all.begin(), all.end());
        return true;
    }
    return false;
}

bool lms_refer_config_struct::get_publish(std::vector<DString> &value)
{
    if (!publish.empty()) {
        value.assign(publish.begin(), publish.end());
        return true;
    }
    return false;
}

bool lms_refer_config_struct::get_play(std::vector<DString> &value)
{
    if (!play.empty()) {
        value.assign(play.begin(), play.end());
        return true;
    }
    return false;
}

/*****************************************************************************/

lms_hook_config_struct::lms_hook_config_struct()
    : exist_timeout(false)
{

}

lms_hook_config_struct::~lms_hook_config_struct()
{

}

void lms_hook_config_struct::load_config(lms_config_directive *directive)
{
    if (true) {
        lms_config_directive *conf = directive->get("on_rtmp_connect");
        if (conf && conf->args.size() == 2) {
            connect_pattern = conf->arg(0);

            connect = conf->arg(1);

            log_trace("connect_pattern=%s, connect=%s", connect_pattern.c_str(), connect.c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("on_rtmp_publish");
        if (conf && conf->args.size() == 2) {
            publish_pattern = conf->arg(0);

            publish = conf->arg(1);

            log_trace("publish_pattern=%s, publish=%s", publish_pattern.c_str(), publish.c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("on_rtmp_play");
        if (conf && conf->args.size() == 2) {
            play_pattern = conf->arg(0);

            play = conf->arg(1);

            log_trace("play_pattern=%s, play=%s", play_pattern.c_str(), play.c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("on_rtmp_unpublish");
        if (conf && !conf->arg(0).isEmpty()) {
            unpublish = conf->arg(0);

            log_trace("unpublish=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("on_rtmp_stop");
        if (conf && !conf->arg(0).isEmpty()) {
            stop = conf->arg(0);

            log_trace("stop=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("timeout");

        if (conf && !conf->arg(0).isEmpty()) {
            timeout = conf->arg(0).toInt();

            exist_timeout = true;

            log_trace("timeout=%s", conf->arg(0).c_str());
        }
    }
}

lms_hook_config_struct *lms_hook_config_struct::copy()
{
    lms_hook_config_struct *hook = new lms_hook_config_struct();

    hook->connect_pattern = connect_pattern;
    hook->connect = connect;

    hook->publish_pattern = publish_pattern;
    hook->publish = publish;

    hook->play_pattern = play_pattern;
    hook->play = play;

    hook->unpublish = unpublish;
    hook->stop = stop;

    hook->timeout = timeout;

    return hook;
}

bool lms_hook_config_struct::get_connect_pattern(DString &value)
{
    if (connect_pattern.empty()) {
        return false;
    }

    value = connect_pattern;
    return true;
}

bool lms_hook_config_struct::get_connect(DString &value)
{
    if (connect.empty()) {
        return false;
    }

    value = connect;
    return true;
}

bool lms_hook_config_struct::get_publish_pattern(DString &value)
{
    if (publish_pattern.empty()) {
        return false;
    }

    value = publish_pattern;
    return true;
}

bool lms_hook_config_struct::get_publish(DString &value)
{
    if (publish.empty()) {
        return false;
    }

    value = publish;
    return true;
}

bool lms_hook_config_struct::get_play_pattern(DString &value)
{
    if (play_pattern.empty()) {
        return false;
    }

    value = play_pattern;
    return true;
}

bool lms_hook_config_struct::get_play(DString &value)
{
    if (play.empty()) {
        return false;
    }

    value = play;
    return true;
}

bool lms_hook_config_struct::get_unpublish(DString &value)
{
    if (unpublish.empty()) {
        return false;
    }

    value = unpublish;
    return true;
}

bool lms_hook_config_struct::get_stop(DString &value)
{
    if (stop.empty()) {
        return false;
    }

    value = stop;
    return true;
}

bool lms_hook_config_struct::get_timeout(int &time)
{
    if (exist_timeout) {
        time = timeout;
    }
    return exist_timeout;
}

/*****************************************************************************/

lms_location_config_struct::lms_location_config_struct()
    : rtmp(NULL)
    , live(NULL)
    , proxy(NULL)
    , http(NULL)
    , refer(NULL)
    , hook(NULL)
    , hls(NULL)
    , flv(NULL)
{

}

lms_location_config_struct::~lms_location_config_struct()
{
    DFree(rtmp);
    DFree(live);
    DFree(proxy);
    DFree(http);
    DFree(refer);
    DFree(hook);
    DFree(hls);
    DFree(flv);
}

void lms_location_config_struct::load_config(lms_config_directive *directive)
{
    if (directive->args.size() != 2) {
        return;
    }

    type = directive->arg(0);
    pattern = directive->arg(1);

    log_trace("type=%s, pattern=%s", type.c_str(), pattern.c_str());

    if (true) {
        lms_config_directive *conf = directive->get("rtmp");
        if (conf) {
            rtmp = new lms_rtmp_config_struct();
            rtmp->load_config(conf);
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("live");
        if (conf) {
            live = new lms_live_config_struct();
            live->load_config(conf);
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("proxy");
        if (conf) {
            proxy = new lms_proxy_config_struct();
            proxy->load_config(conf);
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("http");
        if (conf) {
            http = new lms_http_config_struct();
            http->load_config(conf);
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("refer");
        if (conf) {
            refer = new lms_refer_config_struct();
            refer->load_config(conf);
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("hook");
        if (conf) {
            hook = new lms_hook_config_struct();
            hook->load_config(conf);
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("hls");
        if (conf) {
            hls = new lms_hls_config_struct();
            hls->load_config(conf);
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("flv");
        if (conf) {
            flv = new lms_flv_dvr_config_struct();
            flv->load_config(conf);
        }
    }
}

lms_location_config_struct *lms_location_config_struct::copy()
{
    lms_location_config_struct *location = new lms_location_config_struct();
    location->pattern = pattern;
    location->type = type;

    if (rtmp) {
        location->rtmp = rtmp->copy();
    }
    if (live) {
        location->live = live->copy();
    }
    if (proxy) {
        location->proxy = proxy->copy();
    }
    if (http) {
        location->http = http->copy();
    }
    if (refer) {
        location->refer = refer->copy();
    }
    if (hook) {
        location->hook = hook->copy();
    }
    if (hls) {
        location->hls = hls->copy();
    }
    if (flv) {
        location->flv = flv->copy();
    }

    return location;
}

bool lms_location_config_struct::get_matched(kernel_request *req)
{
    DString location;
    if (req->app.empty()) {
        location = req->stream;
    } else {
        location = req->app + "/" + req->stream;
    }

    if (type == "=") {
        if (location == pattern) {
            return true;
        }
    } else if (type == "^~") {
        if (location.startWith(pattern)) {
            return true;
        }
    } else if (type == "~") {
        DRegExp regex(pattern, false);
        return regex.execMatch(location);
    } else if (type == "~*") {
        DRegExp regex(pattern, true);
        return regex.execMatch(location);
    }

    return false;
}

bool lms_location_config_struct::get_rtmp_enable(bool &value)
{
    if (rtmp) {
        return rtmp->get_enable(value);
    }

    return false;
}

bool lms_location_config_struct::get_rtmp_chunk_size(int &value)
{
    if (rtmp) {
        return rtmp->get_chunk_size(value);
    }

    return false;
}

bool lms_location_config_struct::get_rtmp_in_ack_size(int &value)
{
    if (rtmp) {
        return rtmp->get_in_ack_size(value);
    }

    return false;
}

bool lms_location_config_struct::get_time_jitter(bool &value)
{
    if (live) {
        return live->get_jitter(value);
    }

    return false;
}

bool lms_location_config_struct::get_time_jitter_type(int &value)
{
    if (live) {
        return live->get_jitter_type(value);
    }

    return false;
}

bool lms_location_config_struct::get_gop_cache(bool &value)
{
    if (live) {
        return live->get_gop_cache(value);
    }

    return false;
}

bool lms_location_config_struct::get_fast_gop(bool &value)
{
    if (live) {
        return live->get_fast_gop(value);
    }

    return false;
}

bool lms_location_config_struct::get_rtmp_timeout(int &value)
{
    if (rtmp) {
        return rtmp->get_timeout(value);
    }

    return false;
}

bool lms_location_config_struct::get_queue_size(int &value)
{
    if (live) {
        return live->get_queue_size(value);
    }

    return false;
}

bool lms_location_config_struct::get_proxy_enable(bool &value)
{
    if (proxy) {
        return proxy->get_proxy_enable(value);
    }

    return false;
}

bool lms_location_config_struct::get_proxy_type(DString &value)
{
    if (proxy) {
        return proxy->get_proxy_type(value);
    }

    return false;
}

bool lms_location_config_struct::get_proxy_pass(std::vector<DString> &value)
{
    if (proxy) {
        return proxy->get_proxy_pass(value);
    }

    return false;
}

bool lms_location_config_struct::get_proxy_vhost(DString &value)
{
    if (proxy) {
        return proxy->get_proxy_vhost(value);
    }

    return false;
}

bool lms_location_config_struct::get_proxy_app(DString &value)
{
    if (proxy) {
        return proxy->get_proxy_app(value);
    }

    return false;
}

bool lms_location_config_struct::get_proxy_stream(DString &value)
{
    if (proxy) {
        return proxy->get_proxy_stream(value);
    }

    return false;
}

bool lms_location_config_struct::get_proxy_timeout(int &value)
{
    if (proxy) {
        return proxy->get_proxy_timeout(value);
    }

    return false;
}

bool lms_location_config_struct::get_proxy_ts_acodec(DString &value)
{
    if (proxy) {
        return proxy->get_proxy_ts_acodec(value);
    }

    return false;
}

bool lms_location_config_struct::get_proxy_ts_vcodec(DString &value)
{
    if (proxy) {
        return proxy->get_proxy_ts_vcodec(value);
    }

    return false;
}

bool lms_location_config_struct::get_http_enable(bool &value)
{
    if (http) {
        return http->get_enable(value);
    }

    return false;
}

bool lms_location_config_struct::get_http_buffer_length(int &value)
{
    if (http) {
        return http->get_buffer_length(value);
    }

    return false;
}

bool lms_location_config_struct::get_http_chunked(bool &value)
{
    if (http) {
        return http->get_chunked(value);
    }

    return false;
}

bool lms_location_config_struct::get_http_root(DString &value)
{
    if (http) {
        return http->get_root(value);
    }

    return false;
}

bool lms_location_config_struct::get_http_timeout(int &value)
{
    if (http) {
        return http->get_timeout(value);
    }

    return false;
}

bool lms_location_config_struct::get_flv_live_enable(bool &value)
{
    if (http) {
        return http->get_flv_live_enable(value);
    }

    return false;
}

bool lms_location_config_struct::get_flv_recv_enable(bool &value)
{
    if (http) {
        return http->get_flv_recv_enable(value);
    }

    return false;
}

bool lms_location_config_struct::get_ts_live_enable(bool &value)
{
    if (http) {
        return http->get_ts_live_enable(value);
    }

    return false;
}

bool lms_location_config_struct::get_ts_live_acodec(DString &value)
{
    if (http) {
        return http->get_ts_live_acodec(value);
    }

    return false;
}

bool lms_location_config_struct::get_ts_live_vcodec(DString &value)
{
    if (http) {
        return http->get_ts_live_vcodec(value);
    }

    return false;
}

bool lms_location_config_struct::get_ts_recv_enable(bool &value)
{
    if (http) {
        return http->get_ts_recv_enable(value);
    }

    return false;
}

bool lms_location_config_struct::get_refer_enable(bool &value)
{
    if (refer) {
        if (refer->get_enable(value)) {
            return true;
        }
    }

    return false;
}

bool lms_location_config_struct::get_refer_all(std::vector<DString> &value)
{
    if (refer) {
        return refer->get_all(value);
    }

    return false;
}

bool lms_location_config_struct::get_refer_publish(std::vector<DString> &value)
{
    if (refer) {
        return refer->get_publish(value);
    }

    return false;
}

bool lms_location_config_struct::get_refer_play(std::vector<DString> &value)
{
    if (refer) {
        return refer->get_play(value);
    }

    return false;
}

bool lms_location_config_struct::get_hook_rtmp_connect(DString &value)
{
    if (hook) {
        return hook->get_connect(value);
    }

    return false;
}

bool lms_location_config_struct::get_hook_rtmp_connect_pattern(DString &value)
{
    if (hook) {
        return hook->get_connect_pattern(value);
    }

    return false;
}

bool lms_location_config_struct::get_hook_rtmp_publish(DString &value)
{
    if (hook) {
        return hook->get_publish(value);
    }

    return false;
}

bool lms_location_config_struct::get_hook_rtmp_publish_pattern(DString &value)
{
    if (hook) {
        return hook->get_publish_pattern(value);
    }

    return false;
}

bool lms_location_config_struct::get_hook_rtmp_play(DString &value)
{
    if (hook) {
        return hook->get_play(value);
    }

    return false;
}

bool lms_location_config_struct::get_hook_rtmp_play_pattern(DString &value)
{
    if (hook) {
        return hook->get_play_pattern(value);
    }

    return false;
}

bool lms_location_config_struct::get_hook_rtmp_unpublish(DString &value)
{
    if (hook) {
        return hook->get_unpublish(value);
    }

    return false;
}

bool lms_location_config_struct::get_hook_rtmp_stop(DString &value)
{
    if (hook) {
        return hook->get_stop(value);
    }

    return false;
}

bool lms_location_config_struct::get_hook_timeout(int &value)
{
    if (hook) {
        if (hook->get_timeout(value)) {
            return true;
        }
    }

    return false;
}

bool lms_location_config_struct::get_hls_enable(bool &value)
{
    if (hls) {
        if (hls->get_enable(value)) {
            return true;
        }
    }

    return false;
}

bool lms_location_config_struct::get_hls_window(double &value)
{
    if (hls) {
        if (hls->get_window(value)) {
            return true;
        }
    }

    return false;
}

bool lms_location_config_struct::get_hls_fragment(double &value)
{
    if (hls) {
        if (hls->get_fragment(value)) {
            return true;
        }
    }

    return false;
}

bool lms_location_config_struct::get_hls_acodec(DString &value)
{
    if (hls) {
        if (hls->get_acodec(value)) {
            return true;
        }
    }

    return false;
}

bool lms_location_config_struct::get_hls_vcodec(DString &value)
{
    if (hls) {
        if (hls->get_vcodec(value)) {
            return true;
        }
    }

    return false;
}

bool lms_location_config_struct::get_hls_m3u8_path(DString &value)
{
    if (hls) {
        if (hls->get_m3u8_path(value)) {
            return true;
        }
    }

    return false;
}

bool lms_location_config_struct::get_hls_ts_path(DString &value)
{
    if (hls) {
        if (hls->get_ts_path(value)) {
            return true;
        }
    }

    return false;
}

bool lms_location_config_struct::get_hls_time_jitter(bool &value)
{
    if (hls) {
        if (hls->get_time_jitter(value)) {
            return true;
        }
    }

    return false;
}

bool lms_location_config_struct::get_hls_time_jitter_type(int &value)
{
    if (hls) {
        if (hls->get_time_jitter_type(value)) {
            return true;
        }
    }

    return false;
}

bool lms_location_config_struct::get_hls_root(DString &value)
{
    if (hls) {
        if (hls->get_root(value)) {
            return true;
        }
    }

    return false;
}

bool lms_location_config_struct::get_hls_time_expired(int &value)
{
    if (hls) {
        if (hls->get_time_expired(value)) {
            return true;
        }
    }

    return false;
}

bool lms_location_config_struct::get_flv_enable(bool &value)
{
    if (flv) {
        if (flv->get_enable(value)) {
            return true;
        }
    }

    return false;
}

bool lms_location_config_struct::get_flv_fragment(double &value)
{
    if (flv) {
        if (flv->get_fragment(value)) {
            return true;
        }
    }

    return false;
}

bool lms_location_config_struct::get_flv_path(DString &value)
{
    if (flv) {
        if (flv->get_path(value)) {
            return true;
        }
    }

    return false;
}

bool lms_location_config_struct::get_flv_time_jitter(bool &value)
{
    if (flv) {
        if (flv->get_time_jitter(value)) {
            return true;
        }
    }

    return false;
}

bool lms_location_config_struct::get_flv_time_jitter_type(int &value)
{
    if (flv) {
        if (flv->get_time_jitter_type(value)) {
            return true;
        }
    }

    return false;
}

bool lms_location_config_struct::get_flv_root(DString &value)
{
    if (flv) {
        if (flv->get_root(value)) {
            return true;
        }
    }

    return false;
}

bool lms_location_config_struct::get_flv_time_expired(int &value)
{
    if (flv) {
        if (flv->get_time_expired(value)) {
            return true;
        }
    }

    return false;
}

/*****************************************************************************/

lms_server_config_struct::lms_server_config_struct()
    : rtmp(NULL)
    , live(NULL)
    , proxy(NULL)
    , http(NULL)
    , refer(NULL)
    , hook(NULL)
    , hls(NULL)
    , flv(NULL)
{

}

lms_server_config_struct::~lms_server_config_struct()
{
    DFree(rtmp);
    DFree(live);
    DFree(proxy);
    DFree(http);
    DFree(refer);
    DFree(hook);
    DFree(hls);
    DFree(flv);

    for (int i = 0; i < (int)locations.size(); ++i) {
        DFree(locations.at(i));
    }
}

void lms_server_config_struct::load_config(lms_config_directive *directive)
{
    if (true) {
        lms_config_directive *conf = directive->get("server_name");
        if (conf) {
            for (int i = 0; i < (int)conf->args.size(); i++) {
                server_name.push_back(conf->args.at(i));

                log_trace("server_name=%s", conf->args.at(i).c_str());
            }
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("rtmp");

        if (conf) {
            rtmp = new lms_rtmp_config_struct();
            rtmp->load_config(conf);
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("live");

        if (conf) {
            live = new lms_live_config_struct();
            live->load_config(conf);
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("proxy");

        if (conf) {
            proxy = new lms_proxy_config_struct();
            proxy->load_config(conf);
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("http");

        if (conf) {
            http = new lms_http_config_struct();
            http->load_config(conf);
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("refer");

        if (conf) {
            refer = new lms_refer_config_struct();
            refer->load_config(conf);
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("hook");

        if (conf) {
            hook = new lms_hook_config_struct();
            hook->load_config(conf);
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("hls");

        if (conf) {
            hls = new lms_hls_config_struct();
            hls->load_config(conf);
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("flv");

        if (conf) {
            flv = new lms_flv_dvr_config_struct();
            flv->load_config(conf);
        }
    }

    if (true) {
        std::vector<lms_config_directive*>::iterator it;

        for (it = directive->directives.begin(); it != directive->directives.end(); ++it) {
            lms_config_directive* conf = *it;

            if (conf->name == "location") {
                lms_location_config_struct *location = new lms_location_config_struct();
                location->load_config(conf);
                locations.push_back(location);
            }
        }
    }
}

lms_server_config_struct *lms_server_config_struct::copy()
{
    lms_server_config_struct *server = new lms_server_config_struct();

    if (!server_name.empty()) {
        server->server_name.assign(server_name.begin(), server_name.end());
    }

    if (rtmp) {
        server->rtmp = rtmp->copy();
    }
    if (live) {
        server->live = live->copy();
    }
    if (proxy) {
        server->proxy = proxy->copy();
    }
    if (http) {
        server->http = http->copy();
    }
    if (refer) {
        server->refer = refer->copy();
    }
    if (hook) {
        server->hook = hook->copy();
    }
    if (hls) {
        server->hls = hls->copy();
    }
    if (flv) {
        server->flv = flv->copy();
    }

    for (int i = 0; i < (int)locations.size(); ++i) {
        server->locations.push_back(locations.at(i)->copy());
    }

    return server;
}

bool lms_server_config_struct::get_matched(kernel_request *req)
{
    for(int i = 0; i < (int)server_name.size(); ++i) {
        DRegExp regex(server_name.at(i));
        if (regex.execMatch(req->vhost)) {
            return true;
        }
    }

    return false;
}

bool lms_server_config_struct::get_rtmp_enable(kernel_request *req)
{
    bool ret = true;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_rtmp_enable(ret)) {
                return ret;
            }
            break;
        }
    }

    if (rtmp) {
        rtmp->get_enable(ret);
    }

    return ret;
}

int lms_server_config_struct::get_rtmp_chunk_size(kernel_request *req)
{
    int ret = 4096;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_rtmp_chunk_size(ret)) {
                return ret;
            }
            break;
        }
    }

    if (rtmp) {
        rtmp->get_chunk_size(ret);
    }

    return ret;
}

int lms_server_config_struct::get_rtmp_in_ack_size(kernel_request *req)
{
    int ret = 0;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_rtmp_in_ack_size(ret)) {
                return ret;
            }
            break;
        }
    }

    if (rtmp) {
        rtmp->get_in_ack_size(ret);
    }

    return ret;
}

bool lms_server_config_struct::get_time_jitter(kernel_request *req)
{
    bool ret = true;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_time_jitter(ret)) {
                return ret;
            }
            break;
        }
    }

    if (live) {
        live->get_jitter(ret);
    }

    return ret;
}

int lms_server_config_struct::get_time_jitter_type(kernel_request *req)
{
    int ret = LmsTimeStamp::middle;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_time_jitter_type(ret)) {
                return ret;
            }
            break;
        }
    }

    if (live) {
        live->get_jitter_type(ret);
    }

    return ret;
}

bool lms_server_config_struct::get_gop_cache(kernel_request *req)
{
    bool ret = true;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_gop_cache(ret)) {
                return ret;
            }
            break;
        }
    }

    if (live) {
        live->get_gop_cache(ret);
    }

    return ret;
}

bool lms_server_config_struct::get_fast_gop(kernel_request *req)
{
    bool ret = false;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_fast_gop(ret)) {
                return ret;
            }
            break;
        }
    }

    if (live) {
        live->get_fast_gop(ret);
    }

    return ret;
}

int lms_server_config_struct::get_rtmp_timeout(kernel_request *req)
{
    int ret = 30;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_rtmp_timeout(ret)) {
                return ret;
            }
            break;
        }
    }

    if (rtmp) {
        rtmp->get_timeout(ret);
    }

    return ret;
}

int lms_server_config_struct::get_queue_size(kernel_request *req)
{
    int ret = 30;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_queue_size(ret)) {
                return ret;
            }
            break;
        }
    }

    if (live) {
        live->get_queue_size(ret);
    }

    return ret;
}

bool lms_server_config_struct::get_proxy_enable(kernel_request *req)
{
    bool ret = false;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_proxy_enable(ret)) {
                return ret;
            }
            break;
        }
    }

    if (proxy) {
        proxy->get_proxy_enable(ret);
    }

    return ret;
}

DString lms_server_config_struct::get_proxy_type(kernel_request *req)
{
    DString ret = "rtmp";

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_proxy_type(ret)) {
                return ret;
            }
            break;
        }
    }

    if (proxy) {
        proxy->get_proxy_type(ret);
    }

    return ret;
}

std::vector<DString> lms_server_config_struct::get_proxy_pass(kernel_request *req)
{
    std::vector<DString> ret;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_proxy_pass(ret)) {
                return ret;
            }
            break;
        }
    }

    if (proxy) {
        proxy->get_proxy_pass(ret);
    }

    return ret;
}

DString lms_server_config_struct::get_proxy_vhost(kernel_request *req)
{
    DString ret = "[vhost]";

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_proxy_vhost(ret)) {
                return ret;
            }
            break;
        }
    }

    if (proxy) {
        proxy->get_proxy_vhost(ret);
    }

    return ret;
}

DString lms_server_config_struct::get_proxy_app(kernel_request *req)
{
    DString ret = "[app]";

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_proxy_app(ret)) {
                return ret;
            }
            break;
        }
    }

    if (proxy) {
        proxy->get_proxy_app(ret);
    }

    return ret;
}

DString lms_server_config_struct::get_proxy_stream(kernel_request *req)
{
    DString ret = "[stream]";

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_proxy_stream(ret)) {
                return ret;
            }
            break;
        }
    }

    if (proxy) {
        proxy->get_proxy_stream(ret);
    }

    return ret;
}

int lms_server_config_struct::get_proxy_timeout(kernel_request *req)
{
    int ret = 10;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_proxy_timeout(ret)) {
                return ret;
            }
            break;
        }
    }

    if (proxy) {
        proxy->get_proxy_timeout(ret);
    }

    return ret;
}

DString lms_server_config_struct::get_proxy_ts_acodec(kernel_request *req)
{
    DString ret = "aac";

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_proxy_ts_acodec(ret)) {
                return ret;
            }
            break;
        }
    }

    if (proxy) {
        proxy->get_proxy_ts_acodec(ret);
    }

    return ret;
}

DString lms_server_config_struct::get_proxy_ts_vcodec(kernel_request *req)
{
    DString ret = "h264";

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_proxy_ts_vcodec(ret)) {
                return ret;
            }
            break;
        }
    }

    if (proxy) {
        proxy->get_proxy_ts_vcodec(ret);
    }

    return ret;
}

bool lms_server_config_struct::get_http_enable(kernel_request *req)
{
    bool ret = true;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_http_enable(ret)) {
                return ret;
            }
            break;
        }
    }

    if (http) {
        http->get_enable(ret);
    }

    return ret;
}

int lms_server_config_struct::get_http_buffer_length(kernel_request *req)
{
    int ret = 3000;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_http_buffer_length(ret)) {
                return ret;
            }
            break;
        }
    }

    if (http) {
        http->get_buffer_length(ret);
    }

    return ret;
}

bool lms_server_config_struct::get_http_chunked(kernel_request *req)
{
    bool ret = true;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_http_chunked(ret)) {
                return ret;
            }
            break;
        }
    }

    if (http) {
        http->get_chunked(ret);
    }

    return ret;
}

DString lms_server_config_struct::get_http_root(kernel_request *req)
{
    DString ret = "html";

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_http_root(ret)) {
                return ret;
            }
            break;
        }
    }

    if (http) {
        http->get_root(ret);
    }

    return ret;
}

int lms_server_config_struct::get_http_timeout(kernel_request *req)
{
    int ret = 30;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_http_timeout(ret)) {
                return ret;
            }
            break;
        }
    }

    if (http) {
        http->get_timeout(ret);
    }

    return ret;
}

bool lms_server_config_struct::get_flv_live_enable(kernel_request *req)
{
    bool ret = false;

    if (!get_http_enable(req)) {
        return ret;
    }

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_flv_live_enable(ret)) {
                return ret;
            }
            break;
        }
    }

    if (http) {
        http->get_flv_live_enable(ret);
    }

    return ret;
}

bool lms_server_config_struct::get_flv_recv_enable(kernel_request *req)
{
    bool ret = false;

    if (!get_http_enable(req)) {
        return ret;
    }

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_flv_recv_enable(ret)) {
                return ret;
            }
            break;
        }
    }

    if (http) {
        http->get_flv_recv_enable(ret);
    }

    return ret;
}

bool lms_server_config_struct::get_ts_live_enable(kernel_request *req)
{
    bool ret = false;

    if (!get_http_enable(req)) {
        return ret;
    }

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_ts_live_enable(ret)) {
                return ret;
            }
            break;
        }
    }

    if (http) {
        http->get_ts_live_enable(ret);
    }

    return ret;
}

DString lms_server_config_struct::get_ts_live_acodec(kernel_request *req)
{
    DString ret = "aac";

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_ts_live_acodec(ret)) {
                return ret;
            }
            break;
        }
    }

    if (http) {
        http->get_ts_live_acodec(ret);
    }

    return ret;
}

DString lms_server_config_struct::get_ts_live_vcodec(kernel_request *req)
{
    DString ret = "h264";

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_ts_live_vcodec(ret)) {
                return ret;
            }
            break;
        }
    }

    if (http) {
        http->get_ts_live_vcodec(ret);
    }

    return ret;
}

bool lms_server_config_struct::get_ts_recv_enable(kernel_request *req)
{
    bool ret = false;

    if (!get_http_enable(req)) {
        return ret;
    }

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_ts_recv_enable(ret)) {
                return ret;
            }
            break;
        }
    }

    if (http) {
        http->get_ts_recv_enable(ret);
    }

    return ret;
}

bool lms_server_config_struct::get_refer_enable(kernel_request *req)
{
    bool ret = false;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_refer_enable(ret)) {
                return ret;
            }
            break;
        }
    }

    if (refer) {
        refer->get_enable(ret);
    }

    return ret;
}

std::vector<DString> lms_server_config_struct::get_refer_all(kernel_request *req)
{
    std::vector<DString> ret;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_refer_all(ret)) {
                return ret;
            }
            break;
        }
    }

    if (refer) {
        refer->get_all(ret);
    }

    return ret;
}

std::vector<DString> lms_server_config_struct::get_refer_publish(kernel_request *req)
{
    std::vector<DString> ret;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_refer_publish(ret)) {
                return ret;
            }
            break;
        }
    }

    if (refer) {
        refer->get_publish(ret);
    }

    return ret;
}

std::vector<DString> lms_server_config_struct::get_refer_play(kernel_request *req)
{
    std::vector<DString> ret;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_refer_play(ret)) {
                return ret;
            }
            break;
        }
    }

    if (refer) {
        refer->get_play(ret);
    }

    return ret;
}

DString lms_server_config_struct::get_hook_rtmp_connect(kernel_request *req)
{
    DString ret;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_hook_rtmp_connect(ret)) {
                return ret;
            }
            break;
        }
    }

    if (hook) {
        hook->get_connect(ret);
    }

    return ret;
}

DString lms_server_config_struct::get_hook_rtmp_connect_pattern(kernel_request *req)
{
    DString ret;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_hook_rtmp_connect_pattern(ret)) {
                return ret;
            }
            break;
        }
    }

    if (hook) {
        hook->get_connect_pattern(ret);
    }

    return ret;
}

DString lms_server_config_struct::get_hook_rtmp_publish(kernel_request *req)
{
    DString ret;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_hook_rtmp_publish(ret)) {
                return ret;
            }
            break;
        }
    }

    if (hook) {
        hook->get_publish(ret);
    }

    return ret;
}

DString lms_server_config_struct::get_hook_rtmp_publish_pattern(kernel_request *req)
{
    DString ret;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_hook_rtmp_publish_pattern(ret)) {
                return ret;
            }
            break;
        }
    }

    if (hook) {
        hook->get_publish_pattern(ret);
    }

    return ret;
}

DString lms_server_config_struct::get_hook_rtmp_play(kernel_request *req)
{
    DString ret;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_hook_rtmp_play(ret)) {
                return ret;
            }
            break;
        }
    }

    if (hook) {
        hook->get_play(ret);
    }

    return ret;
}

DString lms_server_config_struct::get_hook_rtmp_play_pattern(kernel_request *req)
{
    DString ret;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_hook_rtmp_play_pattern(ret)) {
                return ret;
            }
            break;
        }
    }

    if (hook) {
        hook->get_play_pattern(ret);
    }

    return ret;
}

DString lms_server_config_struct::get_hook_rtmp_unpublish(kernel_request *req)
{
    DString ret;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_hook_rtmp_unpublish(ret)) {
                return ret;
            }
            break;
        }
    }

    if (hook) {
        hook->get_unpublish(ret);
    }

    return ret;
}

DString lms_server_config_struct::get_hook_rtmp_stop(kernel_request *req)
{
    DString ret;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_hook_rtmp_stop(ret)) {
                return ret;
            }
            break;
        }
    }

    if (hook) {
        hook->get_stop(ret);
    }

    return ret;
}

int lms_server_config_struct::get_hook_timeout(kernel_request *req)
{
    int ret = 10;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_hook_timeout(ret)) {
                return ret;
            }
            break;
        }
    }

    if (hook) {
        hook->get_timeout(ret);
    }

    return ret;
}

bool lms_server_config_struct::get_hls_enable(kernel_request *req)
{
    bool ret = false;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_hls_enable(ret)) {
                return ret;
            }
            break;
        }
    }

    if (hls) {
        hls->get_enable(ret);
    }

    return ret;
}

double lms_server_config_struct::get_hls_window(kernel_request *req)
{
    double ret = 3;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_hls_window(ret)) {
                return ret;
            }
            break;
        }
    }

    if (hls) {
        hls->get_window(ret);
    }

    return ret;
}

double lms_server_config_struct::get_hls_fragment(kernel_request *req)
{
    double ret = 3;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_hls_fragment(ret)) {
                return ret;
            }
            break;
        }
    }

    if (hls) {
        hls->get_fragment(ret);
    }

    return ret;
}

DString lms_server_config_struct::get_hls_acodec(kernel_request *req)
{
    DString ret = "aac";

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_hls_acodec(ret)) {
                return ret;
            }
            break;
        }
    }

    if (hls) {
        hls->get_acodec(ret);
    }

    return ret;
}

DString lms_server_config_struct::get_hls_vcodec(kernel_request *req)
{
    DString ret = "h264";

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_hls_vcodec(ret)) {
                return ret;
            }
            break;
        }
    }

    if (hls) {
        hls->get_vcodec(ret);
    }

    return ret;
}

DString lms_server_config_struct::get_hls_m3u8_path(kernel_request *req)
{
    DString ret = "[stream].m3u8";

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_hls_m3u8_path(ret)) {
                return ret;
            }
            break;
        }
    }

    if (hls) {
        hls->get_m3u8_path(ret);
    }

    return ret;
}

DString lms_server_config_struct::get_hls_ts_path(kernel_request *req)
{
    DString ret = "[yyyy]/[MM]/[dd]/[hh]/[mm]/[ss].ts";

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_hls_ts_path(ret)) {
                return ret;
            }
            break;
        }
    }

    if (hls) {
        hls->get_ts_path(ret);
    }

    return ret;
}

bool lms_server_config_struct::get_hls_time_jitter(kernel_request *req)
{
    bool ret = false;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_hls_time_jitter(ret)) {
                return ret;
            }
            break;
        }
    }

    if (hls) {
        hls->get_time_jitter(ret);
    }

    return ret;
}

int lms_server_config_struct::get_hls_time_jitter_type(kernel_request *req)
{
    int ret = LmsTimeStamp::middle;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_hls_time_jitter_type(ret)) {
                return ret;
            }
            break;
        }
    }

    if (hls) {
        hls->get_time_jitter_type(ret);
    }

    return ret;
}

DString lms_server_config_struct::get_hls_root(kernel_request *req)
{
    DString ret = "html";

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_hls_root(ret)) {
                return ret;
            }
            break;
        }
    }

    if (hls) {
        hls->get_root(ret);
    }

    return ret;
}

int lms_server_config_struct::get_hls_time_expired(kernel_request *req)
{
    int ret = 120;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_hls_time_expired(ret)) {
                return ret;
            }
            break;
        }
    }

    if (hls) {
        hls->get_time_expired(ret);
    }

    return ret;
}

bool lms_server_config_struct::get_flv_enable(kernel_request *req)
{
    bool ret = false;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_flv_enable(ret)) {
                return ret;
            }
            break;
        }
    }

    if (flv) {
        flv->get_enable(ret);
    }

    return ret;
}

double lms_server_config_struct::get_flv_fragment(kernel_request *req)
{
    double ret = 3;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_flv_fragment(ret)) {
                return ret;
            }
            break;
        }
    }

    if (flv) {
        flv->get_fragment(ret);
    }

    return ret;
}

DString lms_server_config_struct::get_flv_path(kernel_request *req)
{
    DString ret = "[yyyy]/[MM]/[dd]/[hh]/[mm]/[ss].flv";

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_flv_path(ret)) {
                return ret;
            }
            break;
        }
    }

    if (flv) {
        flv->get_path(ret);
    }

    return ret;
}

bool lms_server_config_struct::get_flv_time_jitter(kernel_request *req)
{
    bool ret = false;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_flv_time_jitter(ret)) {
                return ret;
            }
            break;
        }
    }

    if (flv) {
        flv->get_time_jitter(ret);
    }

    return ret;
}

int lms_server_config_struct::get_flv_time_jitter_type(kernel_request *req)
{
    int ret = LmsTimeStamp::middle;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_flv_time_jitter_type(ret)) {
                return ret;
            }
            break;
        }
    }

    if (flv) {
        flv->get_time_jitter_type(ret);
    }

    return ret;
}

DString lms_server_config_struct::get_flv_root(kernel_request *req)
{
    DString ret = "html";

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_flv_root(ret)) {
                return ret;
            }
            break;
        }
    }

    if (flv) {
        flv->get_root(ret);
    }

    return ret;
}

int lms_server_config_struct::get_flv_time_expired(kernel_request *req)
{
    int ret = 120;

    for (int i = 0; i < (int)locations.size(); ++i) {
        lms_location_config_struct *location = locations.at(i);

        if (location->get_matched(req)) {
            if (location->get_flv_time_expired(ret)) {
                return ret;
            }
            break;
        }
    }

    if (flv) {
        flv->get_time_expired(ret);
    }

    return ret;
}

/*****************************************************************************/

lms_access_log_struct::lms_access_log_struct()
    : enable(false)
    , exist_type(false)
    , exist_path(false)
{

}

lms_access_log_struct::~lms_access_log_struct()
{

}

void lms_access_log_struct::load_config(lms_config_directive *directive)
{
    if (true) {
        lms_config_directive *conf = directive->get("enable");
        if (conf && !conf->arg(0).isEmpty()) {
            if (conf->arg(0) == "on") {
                enable = true;
            }

            log_trace("enable=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("type");
        if (conf && !conf->arg(0).isEmpty()) {
            type = conf->arg(0);

            exist_type = true;

            log_trace("access_type=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("path");
        if (conf && !conf->arg(0).isEmpty()) {
            path = conf->arg(0);

            exist_path = true;

            log_trace("access_path=%s", conf->arg(0).c_str());
        }
    }
}

bool lms_access_log_struct::get_enable()
{
    return enable;
}

DString lms_access_log_struct::get_type()
{
    if (exist_type) {
        return type;
    }

    return "all";
}

DString lms_access_log_struct::get_path()
{
    if (exist_path) {
        return path;
    }

    return DEFAULT_ACCESS_LOGPATH;
}

/*****************************************************************************/

lms_config_struct::lms_config_struct()
    : access_log(NULL)
{

}

lms_config_struct::~lms_config_struct()
{
    for (int i = 0; i < (int)servers.size(); ++i) {
        DFree(servers.at(i));
    }

    DFree(access_log);
}

void lms_config_struct::load_global_config(lms_config_directive *directive)
{
    if (true) {
        daemon = true;

        lms_config_directive *conf = directive->get("daemon");

        if (conf && !conf->arg(0).isEmpty()) {
            if (conf->arg(0) == "off") {
                daemon = false;
            }
            log_trace("daemon=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        worker_count = 1;

        lms_config_directive *conf = directive->get("worker_count");

        if (conf && !conf->arg(0).isEmpty()) {
            worker_count = conf->arg(0).toInt();

            log_trace("worker_count=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        log_to_console = true;

        lms_config_directive *conf = directive->get("log_to_console");
        if (conf && !conf->arg(0).isEmpty()) {
            if (conf->arg(0) == "off") {
                log_to_console = false;
            }
            log_trace("log_to_console=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        log_to_file = false;

        lms_config_directive *conf = directive->get("log_to_file");
        if (conf && !conf->arg(0).isEmpty()) {
            if (conf->arg(0) == "on") {
                log_to_file = true;
            }
            log_trace("log_to_file=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        log_level = DLogLevel::Error;

        lms_config_directive *conf = directive->get("log_level");
        if (conf && !conf->arg(0).isEmpty()) {
            DString level = conf->arg(0);

            if (level == "trace") {
                log_level = DLogLevel::Trace;
            } else if (level == "verbose") {
                log_level = DLogLevel::Verbose;
            } else if (level == "info") {
                log_level = DLogLevel::Info;
            } else if (level == "warn") {
                log_level = DLogLevel::Warn;
            } else if (level == "error") {
                log_level = DLogLevel::Error;
            }

            log_trace("log_level=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        log_path = DEFAULT_LOG_FILEPATH;

        lms_config_directive *conf = directive->get("log_path");
        if (conf && !conf->arg(0).isEmpty()) {
            log_path = conf->arg(0);

            log_trace("log_path=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        log_enable_line = false;

        lms_config_directive *conf = directive->get("log_enable_line");
        if (conf && !conf->arg(0).isEmpty()) {
            if (conf->arg(0) == "on") {
                log_enable_line = true;
            }

            log_trace("log_enable_line=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        log_enable_function = false;

        lms_config_directive *conf = directive->get("log_enable_function");
        if (conf && !conf->arg(0).isEmpty()) {
            if (conf->arg(0) == "on") {
                log_enable_function = true;
            }

            log_trace("log_enable_function=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        log_enable_file = false;

        lms_config_directive *conf = directive->get("log_enable_file");
        if (conf && !conf->arg(0).isEmpty()) {
            if (conf->arg(0) == "on") {
                log_enable_file = true;
            }

            log_trace("log_enable_file=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        mempool_enable = true;

        lms_config_directive *conf = directive->get("mempool_enable");
        if (conf && !conf->arg(0).isEmpty()) {
            if (conf->arg(0) == "off") {
                mempool_enable = false;
            }

            log_trace("mempool_enable=%s", conf->arg(0).c_str());
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("rtmp_listen");
        if (conf) {
            for (int i = 0; i < (int)conf->args.size(); ++i) {
                rtmp_ports.push_back(conf->args.at(i).toInt());

                log_trace("rtmp_ports=%s", conf->args.at(i).c_str());
            }
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("http_listen");
        if (conf) {
            for (int i = 0; i < (int)conf->args.size(); ++i) {
                http_ports.push_back(conf->args.at(i).toInt());

                log_trace("http_ports=%s", conf->args.at(i).c_str());
            }
        }
    }

    if (true) {
        lms_config_directive *conf = directive->get("access_log");

        if (conf) {
            access_log = new lms_access_log_struct();
            access_log->load_config(conf);
        }
    }
}

void lms_config_struct::load_server_config(lms_config_directive *directive)
{
    lms_server_config_struct *server = new lms_server_config_struct();
    server->load_config(directive);
    servers.push_back(server);
}

/*****************************************************************************/

lms_config *lms_config::m_instance = new lms_config();

lms_config::lms_config()
    : m_config(NULL)
{

}

lms_config::~lms_config()
{
    DFree(m_config);
}

lms_config *lms_config::instance()
{
    return m_instance;
}

static DString config_path = "../conf/";
static DString base_file = "lms.conf";

int lms_config::parse_file()
{
    int ret = ERROR_SUCCESS;

    DSpinLocker locker(&m_mutex);

    lms_config_buffer buffer;
    lms_config_directive root;

    if (!buffer.load_file(config_path + base_file)) {
        ret = ERROR_CONFIG_LOAD_FILE;
        log_error("load config file failed. file=%s, ret=%d", base_file.c_str(), ret);
        return ret;
    }

    if ((ret = root.parse(&buffer)) != ERROR_SUCCESS) {
        log_error("parse root directive failed. file=%s, ret=%d", base_file.c_str(), ret);
        return ret;
    }

    DFree(m_config);
    m_config = new lms_config_struct();

    lms_config_directive *child = root.get("child");
    std::vector<lms_config_directive*>::iterator it;
    for (it = child->directives.begin(); it != child->directives.end(); ++it) {
        lms_config_directive* conf = *it;
        DString name = conf->name;

        lms_config_buffer child_buffer;
        lms_config_directive child_directive;

        if (!child_buffer.load_file(config_path + name)) {
            ret = ERROR_CONFIG_LOAD_FILE;
            log_error("load child config file failed. file=%s, ret=%d", name.c_str(), ret);
            return ret;
        }

        if ((ret = child_directive.parse(&child_buffer)) != ERROR_SUCCESS) {
            log_error("parse child directive failed. file=%s, ret=%d", name.c_str(), ret);
            return ret;
        }

        m_config->load_server_config(&child_directive);
    }

    m_config->load_global_config(&root);

    return ret;
}

bool lms_config::get_daemon()
{
    return m_config->daemon;
}

int lms_config::get_worker_count()
{
    return m_config->worker_count;
}

bool lms_config::get_log_to_console()
{
    return m_config->log_to_console;
}

bool lms_config::get_log_to_file()
{
    return m_config->log_to_file;
}

int lms_config::get_log_level()
{
    return m_config->log_level;
}

DString lms_config::get_log_path()
{
    return m_config->log_path;
}

bool lms_config::get_log_enable_line()
{
    return m_config->log_enable_file;
}

bool lms_config::get_log_enable_function()
{
    return m_config->log_enable_function;
}

bool lms_config::get_log_enable_file()
{
    return m_config->log_enable_file;
}

bool lms_config::get_mempool_enable()
{
    return m_config->mempool_enable;
}

std::vector<int> lms_config::get_rtmp_ports()
{
    return m_config->rtmp_ports;
}

std::vector<int> lms_config::get_http_ports()
{
    return m_config->http_ports;
}

lms_server_config_struct *lms_config::get_server(kernel_request *req)
{
    DSpinLocker locker(&m_mutex);

    for (int i = 0; i < (int)m_config->servers.size(); ++i) {
        lms_server_config_struct *server = m_config->servers.at(i);

        if (server->get_matched(req)) {
            return server->copy();
        }
    }

    return NULL;
}

bool lms_config::get_access_log_enable()
{
    return m_config->access_log->get_enable();
}

DString lms_config::get_access_log_type()
{
    return m_config->access_log->get_type();
}

DString lms_config::get_access_log_path()
{
    return m_config->access_log->get_path();
}
