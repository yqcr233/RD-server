#ifndef SERVER_HPP
#define SERVER_HPP

#include "core.hpp"
#include "data_queue.hpp"
#include "encoder_.hpp"
#include "capture.hpp"
#include "texturepool.hpp"
#include "udp_streamer.hpp"
#include "control.hpp"
#include "stream_info.hpp"

class PlayerServer{
    public:
        PlayerServer();
        PlayerServer(const PlayerServer&) = delete;
        PlayerServer(PlayerServer&&) = delete;
        PlayerServer& operator=(const PlayerServer&) = delete;
        PlayerServer& operator=(PlayerServer&&) = delete;
        ~PlayerServer();

        void start();

    private:
        thread* connect_, *capture_, *control_, *encoder_;
        unique_ptr<UDPStreamer> streamers;
        shared_ptr<DataQueue<shared_ptr<AVPacket>>> packets;
        unique_ptr<GPUCapture> captrue;
        unique_ptr<GPUEncoder> encoder;
        shared_ptr<TexturePool> textures;
        shared_ptr<streamInfo> info;

        void connect();
        void initServer();
        void close();
};
#endif