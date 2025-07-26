#include "udp_streamer.hpp"

using namespace std;

UDPStreamer::UDPStreamer(){
    initWinsock();
}
UDPStreamer::~UDPStreamer(){
    closeSocket();
}

void UDPStreamer::initWinsock(){
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2,2), &wsaData);
    if(result != 0){
        cerr << "WSAStartup failed: " + to_string(result) <<endl;
    }

    setupSocket(server, ip, remote_port, client_addr); // 创建视频socket
    setupSocket(control_server, ip, remote_port + 1, control_server_addr); // 创建控制socket
}

void UDPStreamer::setupSocket(SOCKET &socket_, const string ip_, const int port_, sockaddr_in &address_){
    socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(socket_ == INVALID_SOCKET) {
        int err = WSAGetLastError();
        cerr << "Socket creation failed: " + to_string(err) <<endl;
    }
    // 设置推流端阻塞模式
    unsigned long mode = 0;
    ioctlsocket(socket_, FIONBIO, &mode);
    // 设置发送缓冲区大小
    int recv_buf_size = 10 * 1024 * 1024; //10mb
    setsockopt(socket_, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char*>(&recv_buf_size), sizeof(recv_buf_size));
    
    memset(&address_, 0, sizeof(sockaddr_in));
    address_.sin_family = AF_INET;
    address_.sin_port = htons(port_);
    address_.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(socket_, reinterpret_cast<sockaddr*>(&address_), sizeof(address_));
    cout << "UDP socket configured for " + ip_ + ":" + to_string(port_) <<endl;
}

void UDPStreamer::closeSocket(){
    if(server != INVALID_SOCKET) {
        closesocket(server);
        server = INVALID_SOCKET;
    }
    if(control_server != INVALID_SOCKET) {
        closesocket(control_server);
        control_server = INVALID_SOCKET;
    }
    WSACleanup();
} 

void UDPStreamer::initStreamInfo(uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrate){
    info = make_shared<streamInfo>(width, height, fps, bitrate); 
}  

void UDPStreamer::createServer(shared_ptr<DataQueue<shared_ptr<AVPacket>>> packets){
    cout << "start to listen" <<endl;
    int len = sizeof(client_addr);
    char buf[1024];
    { // 通信前协商
        streamInfo info_{0, 0, 0, 0};
        recvfrom(server, reinterpret_cast<char*>(&info_), sizeof(streamInfo), 0, reinterpret_cast<sockaddr*>(&client_addr), &len);
        fprintf(stdout, "%d %d %d %d\n", info_.width, info_.height, info_.fps, info_.bitrate);
        initStreamInfo(info_.width, info_.height, info_.fps, info_.bitrate); // 通信前先协商双方信息
        sendto(server, "ok", sizeof("ok"), 0, reinterpret_cast<sockaddr*>(&client_addr), len);
    }
    while(true){
        memset(buf, 0, sizeof(buf));
        shared_ptr<AVPacket> packet = shared_ptr<AVPacket>(nullptr, [](AVPacket* temp){
            av_packet_free(&temp);
        });
        recvfrom(server, buf, sizeof(buf), 0, reinterpret_cast<sockaddr*>(&client_addr), &len);
        string s(buf);
        if(s == "get") { // 同步发送chunk数据， 防止丢包和数据覆盖丢失
            packets->pop(packet);
            send_chunk_packet(packets, packet);// 帧过大，重新设计为分片发送
        } else if(s == "over") break;
    }
    cout << "connect over" <<endl;
}

void UDPStreamer::send_chunk_packet(shared_ptr<DataQueue<shared_ptr<AVPacket>>> packets,
                                 shared_ptr<AVPacket> packet)
{
    int sz = packet->size;
    unique_ptr<header> h = make_unique<header>();
    int len = sizeof(server_addr);
    static char buf[1024];
    if(sz <= 1452) {
        h->packet_id = ++packet_count;
        h->chunk_idx = 1;
        h->chunk_size = packet->size;
        h->sum_chunk = 1;
        h->packet_size = packet->size;
        vector<char> buffer(h->chunk_size + sizeof(header));
        memcpy(buffer.data(), h.get(), sizeof(header));
        memcpy(buffer.data() + sizeof(header), packet->data, packet->size);
        sendto(server, buffer.data(), buffer.size(), 0, (sockaddr*)&client_addr, sizeof(client_addr));
    }else{
        int offset = 0;
        h->packet_id = ++packet_count;
        h->sum_chunk = ceil(packet->size * 1.0 / 1452);
        h->packet_size = packet->size;
        h->chunk_idx = 1;
        while(offset < packet->size){
            int res = packet->size - offset;
            h->chunk_size = min(packet->size - offset, 1452);
            vector<char> buffer(sizeof(header) + h->chunk_size);
            memcpy(buffer.data(), h.get(), sizeof(header));
            memcpy(buffer.data() + sizeof(header), packet->data + offset, h->chunk_size);
            sendto(server, buffer.data(), buffer.size(), 0, (sockaddr*)&client_addr, sizeof(client_addr));
            h->chunk_idx ++;
            offset += h->chunk_size;
            // if(offset < packet->size)  
            //     recvfrom(server, buf, sizeof(buf), 0, reinterpret_cast<sockaddr*>(&client_addr), &len); // 等待发送下个chunk指令
        }
    }
}

void UDPStreamer::controlOfMK(int src_width, int src_height){
    char buf[1024];
    int len = sizeof(sockaddr);
    int32_t mouse_x = 0, mouse_y = 0; // 实时更新鼠标位置

    while(true) {
        recvfrom(control_server, buf, sizeof(buf), 0, 
                    (sockaddr*)(&control_server_addr), &len);
        uint8_t type;
        memcpy(&type, buf, sizeof(type));
        switch(type) {
            case 1:{ // 鼠标绝对移动事件
                MouseMoveEvent ev = unserializeMouseMove(buf);
                // 转换到windows坐标
                ev.x = ev.x * src_width / ev.width;
                ev.y = ev.y * src_height / ev.height;
                // 归一化到0~65535
                ev.x = (ev.x * 65535.0f) / (src_width - 1);
                ev.y = (ev.y * 65535.0f) / (src_height - 1);
                // 更新鼠标位置
                cout<<(int)ev.event_type<<" "<<(int)ev.x<<" "<<(int)ev.y<<endl;
                mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, ev.x, ev.y, 0, 0);
                break;
            }   
            case 2:
            case 3:{ // 鼠标按键事件
                MouseButtonEvent ev = unserializeMouseButton(buf);
                cout<<(int)ev.event_type<<" "<<(int)ev.down<<" "<<(int)ev.button<<endl;
                break;
            }
            case 4:{ // 鼠标滚轮事件
                MouseWheelEvent ev = unserializeMouseWheel(buf);
                ev.scroll_y *= WHEEL_DELTA; // 1 sdl滚动单位 = 1  wheel_delta(120)
                cout<<(int)ev.event_type<<" "<<(int)ev.scroll_x<<" "<<(int)ev.scroll_y<<endl;
                break;
            }
            case 5:{ // 键盘按键事件
                KeyboardEvent ev = unserialize(buf);
                cout<<(int)ev.event_type<<" "<<(int)ev.state<<" "<<(int)ev.scancode<<endl;
                break;
            }
            default: break;
        }
    }
}

shared_ptr<streamInfo> UDPStreamer::getStreamInfo(){
    return info;
}