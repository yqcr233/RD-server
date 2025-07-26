cmake_minimum_required(VERSION 3.15)
project(GPUEncode)

# 创建功能模块库（也可以是可执行文件）
add_library(gpu STATIC encoder_.cpp)

target_include_directories(gpu PRIVATE ${TOP_DIR}/include)
target_link_libraries(gpu PRIVATE project_includes)
target_link_libraries(gpu PRIVATE queue texturepool project_thirdparty)

