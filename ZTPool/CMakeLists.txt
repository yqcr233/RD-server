cmake_minimum_required(VERSION 3.15)
project(TexturePool)

add_library(texturepool texturepool.cpp)

target_include_directories(texturepool PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(texturepool PRIVATE project_includes)
# target_link_libraries(server PRIVATE project_thirdparty queue gpu capture texturepool udp_streamer control)
