#ifndef LMS_VERIFY_HOOKS_HPP
#define LMS_VERIFY_HOOKS_HPP

#include "DTcpSocket.hpp"
#include "DHostInfo.hpp"
#include "DHttpParser.hpp"
#include "rtmp_global.hpp"
#include "http_uri.hpp"
#include "http_reader.hpp"

typedef std::tr1::function<void (bool)> HookHandlerEvent;
#define HOOK_CALLBACK(str)  std::tr1::bind(str, this, std::tr1::placeholders::_1)

class lms_verify_hooks : public DTcpSocket
{
public:
    lms_verify_hooks(DEvent *event);
    ~lms_verify_hooks();

    void set_timeout(dint64 timeout);
    void set_handler(HookHandlerEvent handler);

    void on_connect(rtmp_request *req, duint64 id, const DString &url, const DString &ip);
    void on_publish(rtmp_request *req, duint64 id, const DString &url, const DString &ip);
    void on_play(rtmp_request *req, duint64 id, const DString &url, const DString &ip);
    void on_unpublish(rtmp_request *req, duint64 id, const DString &url, const DString &ip);
    void on_stop(rtmp_request *req, duint64 id, const DString &url, const DString &ip);

public:
    virtual int onReadProcess();
    virtual int onWriteProcess();
    virtual void onReadTimeOutProcess();
    virtual void onWriteTimeOutProcess();
    virtual void onErrorProcess();
    virtual void onCloseProcess();

private:
    void lookup_host();
    void onHost(const DStringList &ips);

    void do_post();

    void release();

    int read_header();
    int read_body();
    int parse_body();

    void verify();

private:
    DString m_value;
    http_uri m_uri;

    dint8 m_schedule;
    int m_content_length;

    DHttpParser *m_parser;
    http_reader *m_reader;

    HookHandlerEvent m_handler;
    bool m_result;

    bool m_post_started;

    dint64 m_timeout;

    duint64 m_id;
};

#endif // LMS_VERIFY_HOOKS_HPP
