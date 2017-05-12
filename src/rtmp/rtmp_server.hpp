#ifndef RTMP_SERVER_HPP
#define RTMP_SERVER_HPP

#include "DTcpSocket.hpp"
#include "DSharedPtr.hpp"
#include "rtmp_handshake.hpp"
#include "rtmp_chunk.hpp"
#include "rtmp_packet.hpp"
#include "rtmp_global.hpp"

class rtmp_server
{
public:
    rtmp_server(DTcpSocket *socket);
    ~rtmp_server();

public:
    int do_process();

    // 设置自己的chunk size
    void set_chunk_size(dint32 chunk_size);
    rtmp_request *get_request();
    DSharedPtr<ConnectAppPacket> get_connect_packet();
    /**
     * @brief 发送音视频数据
     */
    int send_av_data(CommonMessage *msg);
    /**
     * @brief 音视频数据回调。交给外部回调去处理时需要将msg拷贝一份，因为原始的msg在decode_message完成后会释放
     */
    void set_av_handler(AVHandlerEvent handler);
    /**
     * @brief metadata回调。
     */
    void set_metadata_handler(AVHandlerEvent handler);
    /**
     * @brief 收到connect之后，使用回调进行验证
     */
    void set_connect_verify_handler(NotifyHandlerEvent handler);
    /**
     * @brief 验证play收到的流名
     */
    void set_play_verify_handler(NotifyHandlerEvent handler);
    /**
     * @brief 验证publish收到的流名
     */
    void set_publish_verify_handler(NotifyHandlerEvent handler);
    /**
     * @brief 发送NetStream.play.PublishNotify
     */
    void set_play_start(VerifyHandlerEvent handler);
    /**
     * @brief 发送NetStream.play.PublishNotify
     */
    int send_publish_notify();

    dint64 get_player_buffer_length();

    int response_connect(bool allow);
    int response_publish(bool allow);
    int response_play(bool allow);

private:
    int handshake();
    int read_chunk();
    int decode_message();

    int process_connect_app(CommonMessage *msg);
    int process_create_stream(CommonMessage *msg);
    int process_publish(CommonMessage *msg);
    int process_play(CommonMessage *msg);
    int process_release_stream(CommonMessage *msg);
    int process_FCPublish(CommonMessage *msg);
    int process_FCUnpublish(CommonMessage *msg);
    int process_close_stream(CommonMessage *msg);

    // 处理接收到的set chunk size包
    int process_set_chunk_size(CommonMessage *msg);
    int process_window_ackledgement_size(CommonMessage *msg);
    int process_user_control(CommonMessage *msg);
    int process_metadata(CommonMessage *msg);

    int process_video_audio(CommonMessage *msg);
    int process_aggregate(CommonMessage *msg);

private:
    int send_connect_response();
    int send_connect_refuse();
    int send_window_ack_size(dint32 ack_size);
    int send_chunk_size(dint32 chunk_size);
    int send_acknowledgement(dint32 recv_size);

private:
    int get_command_name(char *data, int len, DString &name);

private:
    DTcpSocket *m_socket;
    rtmp_request *m_req;

    // 握手
    bool m_hs;
    rtmp_handshake *m_hs_handler;

    // 读chunk
    bool m_ch;
    rtmp_chunk_read *m_chunk_reader;
    // 写chunk
    rtmp_chunk_write *m_chunk_writer;

    // 对方发包的chunk size
    dint32 m_in_chunk_size;
    // 我向外发包的chunk size
    dint32 m_out_chunk_size;

    // 收到对端消息大小等于此值时，需要发送acknowedgement进行响应
    dint32 m_window_ack_size;
    // 响应的acknowedgement的大小
    duint64 m_window_response_size;
    // 上一次响应的acknowedgement的大小
    duint64 m_window_last_ack;
    // 到上一次为止，收到的总大小
    duint64 m_window_total_size;

    // 客户端的缓冲区大小
    dint64 m_player_buffer_length;

    double m_objectEncoding;

    // 响应createStream时，发送给客户端，后面的数据全部使用此值，固定为1
    int m_stream_id;

    // connect包
    DSharedPtr<ConnectAppPacket> m_conn_pkt;

private:
    // 处理video、Audio
    AVHandlerEvent m_av_handler;
    AVHandlerEvent m_metadata_handler;

    NotifyHandlerEvent m_connect_notify_handler;
    NotifyHandlerEvent m_play_verify_handler;
    NotifyHandlerEvent m_publish_verify_handler;
    VerifyHandlerEvent m_play_start_handler;
};

#endif // RTMP_SERVER_HPP
