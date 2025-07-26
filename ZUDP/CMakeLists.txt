cmake_minimum_required(VERSION 3.15)
project(UDPStreamer)

add_library(udp_streamer UDPServer.cpp)

target_include_directories(udp_streamer PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(udp_streamer PRIVATE project_includes)
target_link_libraries(udp_streamer PRIVATE project_thirdparty queue control)
