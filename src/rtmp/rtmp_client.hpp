#ifndef RTMP_CLIENT_HPP
#define RTMP_CLIENT_HPP

#include "rtmp_handshake.hpp"
#include "rtmp_chunk.hpp"
#include "rtmp_packet.hpp"
#include "rtmp_global.hpp"
#include "DTcpSocket.hpp"
#include "DSharedPtr.hpp"
#include <map>

namespace RtmpClientSchedule {
    enum _Schedule
    {
        Default = 0,
        Connect,
        ConnectResponsed,
        CreateStream,
        CreateStreamResponsed,
        Publish,
        PublishResponsed,
        play,
        playResponsed
    };
}

class rtmp_client
{
public:
    rtmp_client(DTcpSocket *socket);
    ~rtmp_client();

public:
    /**
     * @brief 从socket读取chunk并处理
     */
    int do_process();
    /**
     * @brief 设置自己的chunk size
     */
    void set_chunk_size(dint32 chunk_size);
    /**
     * @brief 设置自己的缓冲区
     */
    void set_buffer_length(dint64 len);
    /**
     * @brief 做代理时，直接将收到的connect包转发出去
     */
    void set_connect_packet(DSharedPtr<ConnectAppPacket> pkt);
    /**
     * @brief 开始rtmp请求，主动触发handshake
     * @param publish true时说明为推流，false说明为拉流
     */
    int start_rtmp(bool publish, rtmp_request *req);
    /**
     * @brief 发送closeStream
     */
    int stop_rtmp();
    /**
     * @brief 发送音视频数据和metadata
     */
    int send_av_data(CommonMessage *msg);
    /**
     * @brief 接收音视频包时，交给外部回调去处理，处理时需要将msg拷贝一份，因为原始的msg在decode_message完成后会释放
     */
    void set_av_handler(AVHandlerEvent handler);
    /**
     * @brief 设置metadata的处理函数
     */
    void set_metadata_handler(AVHandlerEvent handler);

    bool can_publish();

    void set_timeout(dint64 timeout);

private:
    int read_chunk();
    int decode_message();

    /**
     * @brief 如果握手成功，直接触发connect
     */
    int handshake_and_connect();
    int connect_app();

    int create_stream();
    int publish();
    int play();

private:
    /**
     * @brief 如果处理connect的响应成功，发送setChunkSize，然后createStream
     */
    int process_connect_response(CommonMessage *msg);
    /**
     * @brief 如果处理createStream的响应成功，发送play或publish
     */
    int process_create_stream_response(CommonMessage *msg);
    /**
     * @brief 处理publish或play的响应结果
     */
    int process_command_onstatus(CommonMessage *msg);

    int process_set_chunk_size(CommonMessage *msg);
    int process_window_ackledgement_size(CommonMessage *msg);
    int process_metadata(CommonMessage *msg);

    int process_video_audio(CommonMessage *msg);
    int process_aggregate(CommonMessage *msg);

private:
    int get_command_name_id(char *data, int len, DString &name, double &id);
    int get_command_name(char *data, int len, DString &name);

    int send_chunk_size(dint32 chunk_size);
    int send_acknowledgement(dint32 recv_size);
    int send_buffer_length(dint64 len);

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
    // 对方的chunk size
    dint32 m_in_chunk_size;
    // 我的chunk size
    dint32 m_out_chunk_size;

    // 收到对方消息大小等于此值时需要响应acknowedgement
    dint32 m_window_ack_size;
    // 响应的acknowedgement的大小
    duint64 m_window_response_size;
    // 上一次响应的acknowedgement的大小
    duint64 m_window_last_ack;
    // 到上一次为止，收到的总大小
    duint64 m_window_total_size;

    // 自己缓冲区的大小，默认30秒
    dint64 m_player_buffer_length;
    // createStream响应回来的值，如果是publish，后面全部数据都使用此值
    int m_stream_id;

    bool m_can_publish;

    dint64 m_timeout;

private:
    DSharedPtr<ConnectAppPacket> m_conn_pkt;
    dint32 m_schedule;
    bool m_publish;

    std::map<double, DString> m_requests;

    AVHandlerEvent m_av_handler;
    AVHandlerEvent m_metadata_handler;
};

#endif // RTMP_CLIENT_HPP
