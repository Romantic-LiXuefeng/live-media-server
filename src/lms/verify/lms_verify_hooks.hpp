#ifndef LMS_VERIFY_HOOKS_HPP
#define LMS_VERIFY_HOOKS_HPP

#include "DTcpSocket.hpp"
#include "DHostInfo.hpp"
#include "DHttpParser.hpp"
#include "kernel_request.hpp"
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

    void on_connect(kernel_request *req, duint64 id, const DString &url, const DString &ip);
    void on_publish(kernel_request *req, duint64 id, const DString &url, const DString &ip);
    void on_play(kernel_request *req, duint64 id, const DString &url, const DString &ip);
    void on_unpublish(kernel_request *req, duint64 id, const DString &url, const DString &ip);
    void on_stop(kernel_request *req, duint64 id, const DString &url, const DString &ip);

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
    void onHostError(const DStringList &ips);

    int do_post();

    void release();

    int parse_body();

    void verify();

    int onHttpHeader(DHttpParser *parser);

private:
    DString m_value;
    http_uri m_uri;

    int m_content_length;

    http_reader *m_reader;

    HookHandlerEvent m_handler;
    bool m_result;

    bool m_post_started;

    dint64 m_timeout;

    duint64 m_id;
};

#endif // LMS_VERIFY_HOOKS_HPP
