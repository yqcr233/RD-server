cmake_minimum_required(VERSION 3.15)
project(Control)

add_library(control control.cpp)

target_include_directories(control PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(control PRIVATE project_includes)
# target_link_libraries(udp_streamer PRIVATE project_thirdparty queue control)
