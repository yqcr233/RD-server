cmake_minimum_required(VERSION 3.15)
project(TestModule)

add_library(test test.cpp)

target_include_directories(test PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test PRIVATE project_includes)
