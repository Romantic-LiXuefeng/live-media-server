#ifndef RTMP_CLIENT_HPP
#define RTMP_CLIENT_HPP

#include "rtmp_handshake.hpp"
#include "rtmp_chunk.hpp"
#include "rtmp_packet.hpp"
#include "rtmp_global.hpp"
#include "DTcpSocket.hpp"
#include "DSharedPtr.hpp"
#include "kernel_request.hpp"
#include <map>

class rtmp_client
{
public:
    rtmp_client(DTcpSocket *socket);
    ~rtmp_client();

public:
    /**
     * @brief 从socket读取chunk并处理
     */
    int service();
    /**
     * @brief 设置自己的chunk size
     */
    void set_chunk_size(dint32 chunk_size);

    void set_in_ack_size(dint32 ack_size);

    /**
     * @brief 设置自己的缓冲区
     */
    void set_buffer_length(dint64 len);
    /**
     * @brief 开始rtmp请求，主动触发handshake
     * @param publish true时说明为推流，false说明为拉流
     */
    int start_rtmp(bool publish, kernel_request *req);
    /**
     * @brief 发送closeStream
     */
    int stop_rtmp();
    /**
     * @brief 发送音视频数据和metadata
     */
    int send_av_data(RtmpMessage *msg);
    /**
     * @brief 接收音视频包时，交给外部回调去处理，处理时需要将msg拷贝一份，因为原始的msg在decode_message完成后会释放
     */
    void set_av_handler(RtmpAVHandler handler);
    /**
     * @brief 设置metadata的处理函数
     */
    void set_metadata_handler(RtmpAVHandler handler);

    void set_publish_start_handler(RtmpVerifyHandler handler);
    void set_play_start_handler(RtmpVerifyHandler handler);

private:
    int read_chunk();
    int decode_message();

    /**
     * @brief 如果握手成功，直接触发connect
     */
    int handshake();
    int connect_app();

    int create_stream();
    int publish();
    int play();

private:
    /**
     * @brief 如果处理connect的响应成功，发送setChunkSize，然后createStream
     */
    int process_connect_response(RtmpMessage *msg);
    /**
     * @brief 如果处理createStream的响应成功，发送play或publish
     */
    int process_create_stream_response(RtmpMessage *msg);
    /**
     * @brief 处理publish或play的响应结果
     */
    int process_command_onstatus(RtmpMessage *msg);

    int process_set_chunk_size(RtmpMessage *msg);
    int process_window_ackledgement_size(RtmpMessage *msg);
    int process_metadata(RtmpMessage *msg);

    int process_video_audio(RtmpMessage *msg);
    int process_aggregate(RtmpMessage *msg);

private:
    int get_command_name_id(char *data, int len, DString &name, double &id);
    int get_command_name(char *data, int len, DString &name);

    int send_chunk_size(dint32 chunk_size);
    int send_acknowledgement();
    int send_buffer_length(dint64 len);

    int send_message(RtmpMessage *msg);

private:
    DTcpSocket *m_socket;
    kernel_request *m_req;
    rtmp_handshake *m_hs;
    RtmpChunk *m_ch;

    // 对方的chunk size
    dint32 m_in_chunk_size;
    // 我的chunk size
    dint32 m_out_chunk_size;

    AckWindowSize in_ack_size;

    // 自己缓冲区的大小，默认1秒
    dint64 m_player_buffer_length;
    // createStream响应回来的值，如果是publish，后面全部数据都使用此值
    int m_stream_id;

private:
    bool m_publish;

    std::map<double, DString> m_requests;

    RtmpAVHandler m_av_handler;
    RtmpAVHandler m_metadata_handler;
    RtmpVerifyHandler m_publish_start_handler;
    RtmpVerifyHandler m_play_start_handler;
private:
    enum ClientSchedule
    {
        HandShake = 0,
        Connect,
        CreateStream,
        Publish,
        Play,
        RecvChunk
    };
    dint8 m_type;
};

#endif // RTMP_CLIENT_HPP
