#include "lms_rtmp_server_stream.hpp"
#include "kernel_errno.hpp"
#include "kernel_log.hpp"
#include "lms_verify_refer.hpp"

lms_rtmp_server_stream::lms_rtmp_server_stream(lms_conn_base *parent)
    : lms_server_stream_base(parent)
    , m_type(rtmp_default)
    , m_event(parent->getEvent())
    , m_hook_timeout(-1)
{
    m_rtmp = new rtmp_server(parent);
    m_rtmp->set_connect_verify_handler(Notify_Handler_CALLBACK(&lms_rtmp_server_stream::onConnect));
    m_rtmp->set_play_verify_handler(Notify_Handler_CALLBACK(&lms_rtmp_server_stream::onPlay));
    m_rtmp->set_publish_verify_handler(Notify_Handler_CALLBACK(&lms_rtmp_server_stream::onPublish));
    m_rtmp->set_av_handler(AV_Handler_CALLBACK(&lms_rtmp_server_stream::onMessage));
    m_rtmp->set_metadata_handler(AV_Handler_CALLBACK(&lms_rtmp_server_stream::onMessage));
    m_rtmp->set_play_start(Verify_Handler_CALLBACK(&lms_rtmp_server_stream::onPlayStart));

}

lms_rtmp_server_stream::~lms_rtmp_server_stream()
{
    DFree(m_rtmp);
}

int lms_rtmp_server_stream::do_process()
{
    return m_rtmp->do_process();
}

void lms_rtmp_server_stream::release()
{
    if (!m_source) {
        return;
    }

    if (m_type == rtmp_publish) {
        m_source->onUnpublish();

        if (!m_unpublish_url.empty()) {
            lms_verify_hooks *hook = new lms_verify_hooks(m_event);
            hook->set_timeout(m_hook_timeout);
            hook->on_unpublish(m_req, global_context->get_id(), m_unpublish_url, get_client_ip());
        }
    } else if (m_type == rtmp_play){
        m_source->del_connection(dynamic_cast<lms_conn_base*>(m_parent));

        if (!m_stop_url.empty()) {
            lms_verify_hooks *hook = new lms_verify_hooks(m_event);
            hook->set_timeout(m_hook_timeout);
            hook->on_stop(m_req, global_context->get_id(), m_stop_url, get_client_ip());
        }
    }

    m_source->del_reload(dynamic_cast<lms_conn_base*>(m_parent));
    m_source = NULL;
}

void lms_rtmp_server_stream::reload_self(lms_server_config_struct *config)
{
    if (m_type == rtmp_publish) {
        m_parent->setRecvTimeOut(m_timeout);
    } else if (m_type == rtmp_play){
        m_parent->setSendTimeOut(m_timeout);
    }

    if (!config->get_rtmp_enable(m_req)) {
        m_parent->release();
    }
}

bool lms_rtmp_server_stream::get_self_config(lms_server_config_struct *config)
{
    int chunk_size = config->get_rtmp_chunk_size(m_req);
    m_rtmp->set_chunk_size(chunk_size);

    m_publish_pattern = config->get_hook_rtmp_publish_pattern(m_req);
    m_publish_url = config->get_hook_rtmp_publish(m_req);

    m_play_pattern = config->get_hook_rtmp_play_pattern(m_req);
    m_play_url = config->get_hook_rtmp_play(m_req);

    m_unpublish_url = config->get_hook_rtmp_unpublish(m_req);
    m_stop_url = config->get_hook_rtmp_stop(m_req);

    dint64 timeout = config->get_hook_timeout(m_req);
    m_hook_timeout = timeout * 1000 * 1000;

    return config->get_rtmp_enable(m_req);
}

int lms_rtmp_server_stream::send_av_data(CommonMessage *msg)
{
    return m_rtmp->send_av_data(msg);
}

void lms_rtmp_server_stream::onConnect(rtmp_request *req)
{
    log_trace("rtmp connect. tcUrl=%s, pageUrl=%s, swfUrl=%s, vhost=%s, app=%s",
              req->tcUrl.c_str(), req->pageUrl.c_str(), req->swfUrl.c_str(),
              req->vhost.c_str(), req->app.c_str());

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
                hook->set_handler(HOOK_CALLBACK(&lms_rtmp_server_stream::onVerifyConnectFinished));
            } else {
                onVerifyConnectFinished(true);
            }

            hook->on_connect(req, global_context->get_id(), url, get_client_ip());
        }
    }
}

void lms_rtmp_server_stream::onPlay(rtmp_request *req)
{
    // 验证配置文件，看此app、stream是否可用
    log_trace("client identified, type=play. stream_name=%s", req->stream.c_str());

    if (m_type != rtmp_default) {
        m_parent->release();
        return;
    }

    m_type = rtmp_play;
    m_req->copy(req);

    if (!get_config_value(req)) {
        m_parent->release();
        return;
    }

    // 验证refer防盗链
    lms_verify_refer refer;
    if (!refer.check(req, false)) {
        log_warn("rtmp play referer verify refused");
        m_parent->release();
        return;
    }

    if (m_play_pattern.empty() || m_play_url.empty()) {
        onVerifyPlayFinished(true);
    } else {
        lms_verify_hooks *hook = new lms_verify_hooks(m_event);
        hook->set_timeout(m_hook_timeout);

        if (m_play_pattern == "verify") {
            hook->set_handler(HOOK_CALLBACK(&lms_rtmp_server_stream::onVerifyPlayFinished));
        } else {
            onVerifyPlayFinished(true);
        }

        hook->on_play(req, global_context->get_id(), m_play_url, get_client_ip());
    }
}

void lms_rtmp_server_stream::onPublish(rtmp_request *req)
{
    // 验证配置文件，看此app、stream是否可用
    log_trace("client identified, type=publish. stream_name=%s", req->stream.c_str());

    if (m_type != rtmp_default) {
        m_parent->release();
        return;
    }

    m_type = rtmp_publish;
    m_req->copy(req);

    if (!get_config_value(m_req)) {
        m_parent->release();
        return;
    }

    // 验证refer防盗链
    lms_verify_refer refer;
    if (!refer.check(req, true)) {
        log_warn("rtmp publish referer verify refused");
        m_parent->release();
        return;
    }

    if (m_publish_pattern.empty() || m_publish_url.empty()) {
        onVerifyPublishFinished(true);
    } else {
        lms_verify_hooks *hook = new lms_verify_hooks(m_event);
        hook->set_timeout(m_hook_timeout);

        if (m_publish_pattern == "verify") {
            hook->set_handler(HOOK_CALLBACK(&lms_rtmp_server_stream::onVerifyPublishFinished));
        } else {
            onVerifyPublishFinished(true);
        }

        hook->on_publish(req, global_context->get_id(), m_publish_url, get_client_ip());
    }
}

bool lms_rtmp_server_stream::onPlayStart(rtmp_request *req)
{
    int ret = ERROR_SUCCESS;

    if (!m_source) {
        return false;
    }

    m_source->add_connection(dynamic_cast<lms_conn_base*>(m_parent));

    dint64 length = m_rtmp->get_player_buffer_length();

    if ((ret = send_gop_messages(length)) != ERROR_SUCCESS) {
        log_error("rtmp server send gop messages failed. ret=%d", ret);
        return false;
    }

    return true;
}

void lms_rtmp_server_stream::onVerifyConnectFinished(bool value)
{
    int ret = m_rtmp->response_connect(value);
    if (ret != ERROR_SUCCESS) {
        m_parent->release();
        return;
    }

    if ((ret = m_parent->writeData()) != ERROR_SUCCESS) {
        ret = ERROR_TCP_SOCKET_WRITE_FAILED;
        log_error("response connect write data failed. ret=%d", ret);
        m_parent->release();
        return;
    }
}

void lms_rtmp_server_stream::onVerifyPublishFinished(bool value)
{
    if (value) {
        m_source = lms_source_manager::instance()->addSource(m_req);
        m_source->add_reload(dynamic_cast<lms_conn_base*>(m_parent));

        m_parent->setSendTimeOut(-1);
        m_parent->setRecvTimeOut(m_timeout);

        if (!m_source->onPublish(m_parent->getEvent())) {
            m_parent->release();
            return;
        }
    }

    int ret = m_rtmp->response_publish(value);
    if (ret != ERROR_SUCCESS) {
        m_parent->release();
        return;
    }

    if ((ret = m_parent->writeData()) != ERROR_SUCCESS) {
        ret = ERROR_TCP_SOCKET_WRITE_FAILED;
        log_error("response publish write data failed. ret=%d", ret);
        m_parent->release();
        return;
    }
}

void lms_rtmp_server_stream::onVerifyPlayFinished(bool value)
{
    if (value) {
        m_source = lms_source_manager::instance()->addSource(m_req);
        m_source->add_reload(dynamic_cast<lms_conn_base*>(m_parent));

        m_parent->setRecvTimeOut(-1);
        m_parent->setSendTimeOut(m_timeout);

        if (!m_source->onPlay(m_parent->getEvent())) {
            m_parent->release();
            return;
        }
    }

    int ret = m_rtmp->response_play(value);
    if (ret != ERROR_SUCCESS) {
        m_parent->release();
        return;
    }

    if ((ret = m_parent->writeData()) != ERROR_SUCCESS) {
        ret = ERROR_TCP_SOCKET_WRITE_FAILED;
        log_error("response play write data failed. ret=%d", ret);
        m_parent->release();
        return;
    }
}
