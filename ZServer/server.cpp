#include "server.hpp"

PlayerServer::PlayerServer(){}
PlayerServer::~PlayerServer(){
    close();
}

void PlayerServer::connect(){
    streamers = make_unique<UDPStreamer>();
    packets = shared_ptr<DataQueue<shared_ptr<AVPacket>>>(new DataQueue<shared_ptr<AVPacket>>(), 
        [](DataQueue<shared_ptr<AVPacket>>* q) {
            q->close();
            delete q;
        });
    connect_ = new thread([this] {streamers->createServer(packets);});
    
    while(!streamers.get()->getStreamInfo().get()); 
    info = streamers.get()->getStreamInfo();
}

void PlayerServer::initServer(){
    encoder = make_unique<GPUEncoder>();
    encoder->initEncode(info->width, info->height, info->bitrate, info->fps);
    captrue = make_unique<GPUCapture>(info.get()->fps);
    captrue->init(encoder->getDevice(), encoder->getDeviceContext());
    textures = make_shared<TexturePool>();
}

void PlayerServer::start(){
    connect();
    initServer();
    cout << "start server" <<endl;
    capture_ = new thread([this]{captrue->CaptureScreen(textures, info.get()->width, info.get()->height);});
    encoder_ = new thread([this]{encoder.get()->encode(packets, textures);});
    control_ = new thread([this]{streamers.get()->controlOfMK(captrue->getSrcWidth(), captrue->getSrcHeight());});
}

void PlayerServer::close(){
    if(connect_->joinable()) connect_->join();
    if(capture_->joinable()) capture_->join();
    if(encoder_->joinable()) encoder_->join();
    if(control_->joinable()) control_->join();
}