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

    int init_flv_live(DHttpParser *parser);
    int init_flv_recv(DHttpParser *parser);

    int init_send_file(DHttpParser *parser);

    int init_ts_live(DHttpParser *parser);
    int init_ts_recv(DHttpParser *parser);

    void response_http_header(int code);

    int start_process();

    int start_flv_live();
    int start_flv_recv();
    int start_send_file();
    int start_ts_live();
    int start_ts_recv();

    void clear_http_body();

private:
    http_reader *m_reader;

    dint8 m_type;

    lms_http_flv_live *m_flv_live;
    lms_http_flv_recv *m_flv_recv;
    lms_http_send_file *m_send_file;
    lms_http_ts_live *m_ts_live;
    lms_http_ts_recv *m_ts_recv;
};

#endif // LMS_HTTP_SERVER_CONN_HPP
