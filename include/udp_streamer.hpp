#ifndef UDP_HPP
#define UDP_HPP
#include "core.hpp"
#include "data_queue.hpp"
#include "control.hpp"
#include "stream_info.hpp"

class UDPStreamer{
    public:
        UDPStreamer();
        UDPStreamer(const UDPStreamer&) = delete;
        UDPStreamer(UDPStreamer&&) = delete;
        UDPStreamer& operator=(const UDPStreamer&) = delete;
        UDPStreamer& operator=(UDPStreamer&&) = delete;
        ~UDPStreamer();

        typedef struct header{ // 定义帧头 20bytes
            uint32_t packet_id;
            uint32_t chunk_idx;
            uint32_t sum_chunk;
            uint32_t chunk_size;
            uint32_t packet_size;
        }header;

        void createServer(shared_ptr<DataQueue<shared_ptr<AVPacket>>> packets);
        void initStreamInfo(uint32_t width = 1280, uint32_t height = 800, uint32_t fps = 60, uint32_t bitrate = 20000000);
        shared_ptr<streamInfo> getStreamInfo();
        void send_chunk_packet(shared_ptr<DataQueue<shared_ptr<AVPacket>>> packets,
                                 shared_ptr<AVPacket> packet); // 同步获取一帧分块数据
        void controlOfMK(int src_width, int src_height);

    private:
        SOCKET server = INVALID_SOCKET; 
        const string ip = "192.168.1.11";
        const int remote_port = 12345;
        sockaddr_in server_addr;
        int packet_count = 0;
        shared_ptr<streamInfo> info;
        SOCKET control_server = INVALID_SOCKET;
        sockaddr_in control_server_addr;
        sockaddr_in client_addr;// 保存客户端信息

        void initWinsock();
        void setupSocket(SOCKET &socket_, const string ip_, const int port_, sockaddr_in &address_);
        void closeSocket();
};

#endif