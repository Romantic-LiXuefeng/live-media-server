#include "lms_rtmp_server_conn.hpp"
#include "kernel_log.hpp"
#include "kernel_errno.hpp"
#include "kernel_utility.hpp"
#include "lms_config.hpp"
#include "lms_verify_hooks.hpp"
#include "lms_verify_refer.hpp"
#include "lms_source.hpp"
#include "DDateTime.hpp"
#include "DMd5.hpp"
#include "lms_rtmp_utility.hpp"

lms_rtmp_server_conn::lms_rtmp_server_conn(DThread *parent, DEvent *ev, int fd)
    : lms_conn_base(parent, ev, fd)
    , m_req(NULL)
    , m_source(NULL)
    , m_writer(NULL)
    , m_hook_timeout(30 * 1000 * 1000)
    , m_timeout(30 * 1000 * 1000)
    , m_enable(false)
    , m_is_edge(false)
    , m_type(Default)
    , m_hooking(false)
    , m_need_release(false)
{
    setWriteTimeOut(m_timeout);
    setReadTimeOut(m_timeout);

    m_rtmp = new rtmp_server(this);
    m_rtmp->set_connect_verify_handler(Rtmp_Notify_Handler_Callback(&lms_rtmp_server_conn::onConnect));
    m_rtmp->set_play_verify_handler(Rtmp_Notify_Handler_Callback(&lms_rtmp_server_conn::onPlay));
    m_rtmp->set_publish_verify_handler(Rtmp_Notify_Handler_Callback(&lms_rtmp_server_conn::onPublish));
    m_rtmp->set_av_handler(Rtmp_AV_Handler_Callback(&lms_rtmp_server_conn::onMessage));
    m_rtmp->set_metadata_handler(Rtmp_AV_Handler_Callback(&lms_rtmp_server_conn::onMessage));
    m_rtmp->set_play_start(Rtmp_Verify_Handler_Callback(&lms_rtmp_server_conn::onPlayStart));

    m_client_ip = get_peer_ip(m_fd);
    m_begin_time = DDateTime::currentDate().toString("yyyy-MM-dd hh:mm:ss.ms");
    m_md5 = DMd5::md5(m_client_ip + m_begin_time + DString::number(fd));
}

lms_rtmp_server_conn::~lms_rtmp_server_conn()
{
    log_warn("free --> lms_rtmp_server_conn");

    DFree(m_rtmp);
    DFree(m_writer);
}

int lms_rtmp_server_conn::onReadProcess()
{
    int ret = ERROR_SUCCESS;

    global_context->update_id(m_fd);

    if ((ret = m_rtmp->service()) != ERROR_SUCCESS) {
        return ret;
    }

    return ret;
}

int lms_rtmp_server_conn::onWriteProcess()
{
    int ret = ERROR_SUCCESS;

    global_context->update_id(m_fd);

    if (m_writer) {
        if ((ret = m_writer->flush()) != ERROR_SUCCESS) {
            return ret;
        }
    }

    return ret;
}

void lms_rtmp_server_conn::onReadTimeOutProcess()
{
    log_error("read timeout");
    release();
}

void lms_rtmp_server_conn::onWriteTimeOutProcess()
{
    log_error("write timeout");
    release();
}

void lms_rtmp_server_conn::onErrorProcess()
{
    log_error("socket error");
    release();
}

void lms_rtmp_server_conn::onCloseProcess()
{
    log_error("socket closed");
    release();
}

int lms_rtmp_server_conn::Process(CommonMessage *msg)
{
    if (m_type != Play) {
        return ERROR_RTMP_INVALID_ORDER;
    }

    return m_writer->send(msg);
}

void lms_rtmp_server_conn::reload()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        release();
        return;
    }

    m_enable = config->get_rtmp_enable(m_req);
    if (!m_enable) {
        release();
        return;
    }

    m_publish_pattern = config->get_hook_rtmp_publish_pattern(m_req);
    m_publish_url = config->get_hook_rtmp_publish(m_req);

    m_play_pattern = config->get_hook_rtmp_play_pattern(m_req);
    m_play_url = config->get_hook_rtmp_play(m_req);

    m_unpublish_url = config->get_hook_rtmp_unpublish(m_req);
    m_stop_url = config->get_hook_rtmp_stop(m_req);

    dint64 hook_timeout = config->get_hook_timeout(m_req);
    m_hook_timeout = hook_timeout * 1000 * 1000;

    int timeout = config->get_rtmp_timeout(m_req);
    m_timeout = timeout * 1000 * 1000;

    if (m_type == Publish) {
        setWriteTimeOut(-1);
        setReadTimeOut(m_timeout);
    } else if (m_type == Play) {
        setReadTimeOut(-1);
        setWriteTimeOut(m_timeout);
    }

    if (m_writer) {
        m_writer->reload();
    }
}

void lms_rtmp_server_conn::release()
{
    // connect play publish的验证操作调用了本类的成员函数，因此要保证验证完成之后才release
    if (m_hooking) {
        m_need_release = true;
        return;
    }

    global_context->update_id(m_fd);
    log_trace("rtmp connection close");

    if (m_type == Publish) {
        if (m_source) {
            m_source->stop_external();
            m_source->onUnpublish();
        }

        if (!m_unpublish_url.empty()) {
            lms_verify_hooks *hook = new lms_verify_hooks(m_event);
            hook->set_timeout(m_hook_timeout);
            hook->on_unpublish(m_req, global_context->get_id(), m_unpublish_url, m_client_ip);
        }
    } else if (m_type == Play){
        if (m_source) {
            m_source->del_connection(this);
        }

        if (!m_stop_url.empty()) {
            lms_verify_hooks *hook = new lms_verify_hooks(m_event);
            hook->set_timeout(m_hook_timeout);
            hook->on_stop(m_req, global_context->get_id(), m_stop_url, m_client_ip);
        }
    }

    if (m_source) {
        m_source->del_reload_conn(this);
        m_source = NULL;
    }

    rtmp_access_log_end(m_req, m_client_ip, m_md5);

    global_context->delete_id(m_fd);

    close();
}

void lms_rtmp_server_conn::onConnect(kernel_request *req)
{
    log_trace("rtmp connect. tcUrl=%s, pageUrl=%s, swfUrl=%s, vhost=%s, app=%s",
              req->tcUrl.c_str(), req->pageUrl.c_str(), req->swfUrl.c_str(),
              req->vhost.c_str(), req->app.c_str());

    if (m_type != Default) {
        release();
        return;
    }
    m_type = Connect;

    verify_connect(req);
}

void lms_rtmp_server_conn::onPlay(kernel_request *req)
{
    log_trace("client identified, type=play. stream_name=%s", req->stream.c_str());

    if (m_type != Connect) {
        release();
        return;
    }
    m_type = Play;

    m_req = req;
    get_config_value();

    rtmp_access_log_begin(req, m_begin_time, m_client_ip, m_md5, "play");

    verify_play(m_req);
}

void lms_rtmp_server_conn::onPublish(kernel_request *req)
{
    log_trace("client identified, type=publish. stream_name=%s", req->stream.c_str());

    if (m_type != Connect) {
        release();
        return;
    }
    m_type = Publish;

    m_req = req;
    get_config_value();

    rtmp_access_log_begin(req, m_begin_time, m_client_ip, m_md5, "publish");

    verify_publish(m_req);
}

bool lms_rtmp_server_conn::onPlayStart(kernel_request *req)
{
    if (m_type != Play) {
        return false;
    }

    if (m_writer) {
        return false;
    }

    m_writer = new lms_stream_writer(req, Rtmp_AV_Handler_Callback(&lms_rtmp_server_conn::onSendMessage), false);

    dint64 length = m_rtmp->get_player_buffer_length();

    if (!m_source->add_connection(this)) {
        return false;
    }

    log_trace("player buffer length is %d", length);

    int ret = m_source->send_gop_cache(m_writer, length);
    if ((ret != ERROR_SUCCESS) && (ret != SOCKET_EAGAIN)) {
        return false;
    }

    return true;
}

int lms_rtmp_server_conn::onMessage(RtmpMessage *msg)
{
    if (m_type != Publish) {
        return ERROR_RTMP_INVALID_ORDER;
    }

    CommonMessage *_msg = rtmp_to_common(msg);
    DAutoFree(CommonMessage, _msg);

    if (m_is_edge) {
        return m_source->proxyMessage(_msg);
    } else if (_msg->is_audio()) {
        return m_source->onAudio(_msg);
    } else if (_msg->is_video()) {
        return m_source->onVideo(_msg);
    } else if (_msg->is_metadata()) {
        return m_source->onMetadata(_msg);
    }

    return ERROR_SUCCESS;
}

int lms_rtmp_server_conn::onSendMessage(CommonMessage *msg)
{
    RtmpMessage *_msg = common_to_rtmp(msg);
    DAutoFree(RtmpMessage, _msg);

    return m_rtmp->send_av_data(_msg);
}

void lms_rtmp_server_conn::verify_connect(kernel_request *req)
{
    lms_server_config_struct *config = lms_config::instance()->get_server(req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        onVerifyConnectFinished(false);
    } else {
        DString pattern = config->get_hook_rtmp_connect_pattern(req);
        DString url = config->get_hook_rtmp_connect(req);

        if (pattern.empty() || url.empty()) {
            onVerifyConnectFinished(true);
        } else {
            lms_verify_hooks *hook = new lms_verify_hooks(m_event);
            hook->set_timeout(m_hook_timeout);

            if (pattern == "verify") {
                m_hooking = true;
                hook->set_handler(HOOK_CALLBACK(&lms_rtmp_server_conn::onVerifyConnectFinished));
            } else {
                onVerifyConnectFinished(true);
            }

            hook->on_connect(req, global_context->get_id(), url, m_client_ip);
        }
    }
}

void lms_rtmp_server_conn::onVerifyConnectFinished(bool value)
{
    m_hooking = false;
    if (m_need_release) {
        release();
        return;
    }

    if (m_rtmp->response_connect(value) != ERROR_SUCCESS) {
        release();
        return;
    }
}

void lms_rtmp_server_conn::verify_publish(kernel_request *req)
{
    if (!m_enable) {
        release();
        return;
    }

    // 验证refer防盗链
    lms_verify_refer refer;
    if (!refer.check(req, true)) {
        log_warn("rtmp publish referer verify refused");
        release();
        return;
    }

    if (m_publish_pattern.empty() || m_publish_url.empty()) {
        onVerifyPublishFinished(true);
    } else {
        lms_verify_hooks *hook = new lms_verify_hooks(m_event);
        hook->set_timeout(m_hook_timeout);

        if (m_publish_pattern == "verify") {
            m_hooking = true;
            hook->set_handler(HOOK_CALLBACK(&lms_rtmp_server_conn::onVerifyPublishFinished));
        } else {
            onVerifyPublishFinished(true);
        }

        hook->on_publish(req, global_context->get_id(), m_publish_url, m_client_ip);
    }
}

void lms_rtmp_server_conn::onVerifyPublishFinished(bool value)
{
    m_hooking = false;
    if (m_need_release) {
        release();
        return;
    }

    if (value) {
        m_source = lms_source_manager::instance()->addSource(m_req);

        if (!m_source->add_reload_conn(this)) {
            release();
            return;
        }

        setWriteTimeOut(-1);
        setReadTimeOut(m_timeout);

        if (!m_source->onPublish(m_event, m_is_edge)) {
            release();
            return;
        }

        m_source->start_external();
    }

    if (m_rtmp->response_publish(value) != ERROR_SUCCESS) {
        release();
    }
}

void lms_rtmp_server_conn::verify_play(kernel_request *req)
{
    if (!m_enable) {
        release();
        return;
    }

    // 验证refer防盗链
    lms_verify_refer refer;
    if (!refer.check(req, false)) {
        log_warn("rtmp play referer verify refused");
        release();
        return;
    }

    if (m_play_pattern.empty() || m_play_url.empty()) {
        onVerifyPlayFinished(true);
    } else {
        lms_verify_hooks *hook = new lms_verify_hooks(m_event);
        hook->set_timeout(m_hook_timeout);

        if (m_play_pattern == "verify") {
            m_hooking = true;
            hook->set_handler(HOOK_CALLBACK(&lms_rtmp_server_conn::onVerifyPlayFinished));
        } else {
            onVerifyPlayFinished(true);
        }

        hook->on_play(req, global_context->get_id(), m_play_url, m_client_ip);
    }
}

void lms_rtmp_server_conn::onVerifyPlayFinished(bool value)
{
    m_hooking = false;
    if (m_need_release) {
        release();
        return;
    }

    if (value) {
        m_source = lms_source_manager::instance()->addSource(m_req);

        if (!m_source->add_reload_conn(this)) {
            release();
            return;
        }

        setReadTimeOut(-1);
        setWriteTimeOut(m_timeout);

        if (!m_source->onPlay(m_event, m_is_edge)) {
            release();
            return;
        }
    }

    if (m_rtmp->response_play(value) != ERROR_SUCCESS) {
        release();
    }
}

void lms_rtmp_server_conn::get_config_value()
{
    lms_server_config_struct *config = lms_config::instance()->get_server(m_req);
    DAutoFree(lms_server_config_struct, config);

    if (!config) {
        return;
    }

    int chunk_size = config->get_rtmp_chunk_size(m_req);
    m_rtmp->set_chunk_size(chunk_size);

    int in_ack_size = config->get_rtmp_in_ack_size(m_req);
    m_rtmp->set_in_ack_size(in_ack_size);

    m_publish_pattern = config->get_hook_rtmp_publish_pattern(m_req);
    m_publish_url = config->get_hook_rtmp_publish(m_req);

    m_play_pattern = config->get_hook_rtmp_play_pattern(m_req);
    m_play_url = config->get_hook_rtmp_play(m_req);

    m_unpublish_url = config->get_hook_rtmp_unpublish(m_req);
    m_stop_url = config->get_hook_rtmp_stop(m_req);

    dint64 hook_timeout = config->get_hook_timeout(m_req);
    m_hook_timeout = hook_timeout * 1000 * 1000;

    int timeout = config->get_rtmp_timeout(m_req);
    m_timeout = timeout * 1000 * 1000;

    m_enable = config->get_rtmp_enable(m_req);
    m_is_edge = config->get_proxy_enable(m_req);
}

