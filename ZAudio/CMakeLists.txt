cmake_minimum_required(VERSION 3.15)
project(audio)

add_library(audio audio.cpp)

target_link_libraries(audio PRIVATE project_includes)
