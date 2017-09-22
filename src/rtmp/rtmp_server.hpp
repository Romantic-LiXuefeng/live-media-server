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
    int service();

    // 设置自己的chunk size
    void set_chunk_size(dint32 chunk_size);

    void set_in_ack_size(int ack_size);

    kernel_request *get_request();
    /**
     * @brief 发送音视频数据，只是将数据组合成chunk，放到socket的缓冲区中
     */
    int send_av_data(RtmpMessage *msg);
    /**
     * @brief 音视频数据回调。交给外部回调去处理时需要将msg拷贝一份，因为原始的msg在decode_message完成后会释放
     */
    void set_av_handler(RtmpAVHandler handler);
    /**
     * @brief metadata回调。
     */
    void set_metadata_handler(RtmpAVHandler handler);
    /**
     * @brief 收到connect之后，使用回调进行验证
     */
    void set_connect_verify_handler(RtmpNotifyHandler handler);
    /**
     * @brief 验证play收到的流名
     */
    void set_play_verify_handler(RtmpNotifyHandler handler);
    /**
     * @brief 验证publish收到的流名
     */
    void set_publish_verify_handler(RtmpNotifyHandler handler);
    /**
     * @brief 发送NetStream.play.PublishNotify
     */
    void set_play_start(RtmpVerifyHandler handler);
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

    int process_connect_app(RtmpMessage *msg);
    int process_create_stream(RtmpMessage *msg);
    int process_publish(RtmpMessage *msg);
    int process_play(RtmpMessage *msg);
    int process_release_stream(RtmpMessage *msg);
    int process_FCPublish(RtmpMessage *msg);
    int process_FCUnpublish(RtmpMessage *msg);
    int process_close_stream(RtmpMessage *msg);

    // 处理接收到的set chunk size包
    int process_set_chunk_size(RtmpMessage *msg);
    int process_window_ackledgement_size(RtmpMessage *msg);
    int process_user_control(RtmpMessage *msg);
    int process_metadata(RtmpMessage *msg);

    int process_video_audio(RtmpMessage *msg);
    int process_aggregate(RtmpMessage *msg);

private:
    int send_connect_response();
    int send_connect_refuse();
    int send_window_ack_size(dint32 ack_size);
    int send_chunk_size(dint32 chunk_size);
    int send_acknowledgement();

private:
    int get_command_name(char *data, int len, DString &name);
    int send_message(RtmpMessage *msg);

private:
    DTcpSocket *m_socket;
    kernel_request *m_req;

    rtmp_handshake *m_hs;

    RtmpChunk *m_ch;

    // 对方发包的chunk size
    dint32 m_in_chunk_size;
    // 我向外发包的chunk size
    dint32 m_out_chunk_size;

    AckWindowSize in_ack_size;

    // 客户端的缓冲区大小
    dint64 m_player_buffer_length;

    double m_objectEncoding;

    // 响应createStream时，发送给客户端，后面的数据全部使用此值，固定为1
    int m_stream_id;

private:
    // 处理video、Audio
    RtmpAVHandler m_av_handler;
    RtmpAVHandler m_metadata_handler;

    RtmpNotifyHandler m_connect_notify_handler;
    RtmpNotifyHandler m_play_verify_handler;
    RtmpNotifyHandler m_publish_verify_handler;
    RtmpVerifyHandler m_play_start_handler;

private:
    enum ServerType
    {
        HandShake = 0,
        RecvChunk
    };
    int8_t m_type;

};

#endif // RTMP_SERVER_HPP
