#ifndef INFO_HPP
#define INFO_HPP

#include <cstdint>

struct streamInfo{ // 定义协议协商信息
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    uint32_t bitrate;

    streamInfo(uint32_t width_, uint32_t height_, uint32_t fps_, uint32_t bitrate_):
                        width(width_), height(height_), fps(fps_), bitrate(bitrate_){}

};

#endif