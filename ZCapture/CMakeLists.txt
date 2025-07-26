cmake_minimum_required(VERSION 3.15)
project(Capture)

add_library(capture capture.cpp)

target_include_directories(capture PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(capture PRIVATE project_includes)
target_link_libraries(capture PRIVATE texturepool project_thirdparty)

