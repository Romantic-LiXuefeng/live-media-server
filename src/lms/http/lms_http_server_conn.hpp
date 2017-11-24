#ifndef LMS_HTTP_SERVER_CONN_HPP
#define LMS_HTTP_SERVER_CONN_HPP

#include "lms_conn_base.hpp"
#include "DThread.hpp"
#include "lms_stream_writer.hpp"
#include "http_reader.hpp"
#include "lms_http_flv_live.hpp"
#include "lms_http_flv_recv.hpp"
#include "lms_http_send_file.hpp"
#include "lms_http_ts_live.hpp"
#include "lms_http_ts_recv.hpp"
#include "lms_http_process_base.hpp"

class lms_source;

class lms_http_server_conn : public lms_conn_base
{
public:
    lms_http_server_conn(DThread *parent, DEvent *ev, int fd);
    virtual ~lms_http_server_conn();

    http_reader *reader();

public:
    // Inherited from DTcpSocket
    virtual int  onReadProcess();
    virtual int  onWriteProcess();
    virtual void onReadTimeOutProcess();
    virtual void onWriteTimeOutProcess();
    virtual void onErrorProcess();
    virtual void onCloseProcess();

public:
    // Inherited from lms_conn_base
    virtual int Process(CommonMessage *msg);
    virtual void reload();
    virtual void release();

private:
    int onHttpParser(DHttpParser *parser);
    DString getParam(const DString &param, const DString &key);

    int do_process(DHttpParser *parser);

    void response_http_header(int code);

    void clear_http_body();

private:
    http_reader *m_reader;

    dint8 m_type;

    lms_http_process_base *m_process;

    // 客户端连接上来的时间
    DString m_begin_time;
    // 客户端ip
    DString m_client_ip;
    DString m_md5;

    int m_code;
};

#endif // LMS_HTTP_SERVER_CONN_HPP
