cmake_minimum_required(VERSION 3.15)
project(Server)

add_library(server server.cpp)

target_include_directories(server PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(server PRIVATE project_includes)
target_link_libraries(server PRIVATE project_thirdparty queue gpu capture texturepool udp_streamer control)
