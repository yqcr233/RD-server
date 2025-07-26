cmake_minimum_required(VERSION 3.15)
project(Queue)

add_library(queue data_queue.cpp)

target_include_directories(queue PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(queue PRIVATE project_includes)
target_link_libraries(queue PRIVATE project_thirdparty)
